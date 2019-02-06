#include <stdint.h>
#include <stdbool.h>

#include "printf.h"
#include "panic.h"
#include "regops.h"
#include "object.h"
#include "gic.h"
#include "intc.h"

#define GICD_CTLR		0x0000
#define GICD_TYPER		0x0004
#define GICD_IIDR		0x0008
#define GICD_STATUSR		0x0010
#define GICD_SETSPI_NSR		0x0040
#define GICD_CLRSPI_NSR		0x0048
#define GICD_SETSPI_SR		0x0050
#define GICD_CLRSPI_SR		0x0058
#define GICD_SEIR		0x0068
#define GICD_IGROUPRn		0x0080
#define GICD_ISENABLERn		0x0100
#define GICD_ICENABLERn		0x0180
#define GICD_ISPENDRn		0x0200
#define GICD_ICPENDRn		0x0280
#define GICD_ISACTIVERn		0x0300
#define GICD_ICFGRn             0x0c00
#define GICD_IGROUPMODRn        0x0d00

#define GICR_TYPER              0x0008
#define GICR_WAKER              0x0014

#define GICR_IGROUPRn           0x0080
#define GICR_ISENABLER0         0x0100
#define GICR_ICENABLER0         0x0180
#define GICR_ICFGR0             0x0c00
#define GICR_ICFGR1             0x0c04
#define GICR_IGROUPMODR0        0x0d00

#define GICD_CTRL__ARE          (1 << 4) // when no security extention, use only ARE_S bit
#define GICD_CTRL__ARE_S        (1 << 4)
#define GICD_CTRL__ARE_NS       (1 << 5)

#define GICD_TYPER__IT_LINES_NUMBER__MASK       0xf
#define GICD_TYPER__IT_LINES_NUMBER__SHIFT        0

#define CORE 0 // Everything is for Core #0 for now

// See GIC-500 TRM Section 3.2
#define GIC_ADDR_BITS 19 // 18 + max(1, cel(log2 NUM_CPUS)) = 19
#define GIC_ADDR_MSB (GIC_ADDR_BITS - 1)
#define GICD(r) (r) // Addr [0:00:0000_0000] + r
#define GICR_PPI_SGI(r) ((1 << GIC_ADDR_MSB) + (CORE << 17) + (1 << 16) + r) // Addr [1:CORE:1:0000_0000] + r

#define MAX_IRQS 128

struct irq {
    struct object obj;
    unsigned n;
    gic_irq_type_t type;
    gic_irq_cfg_t cfg;
};

struct gic {
    volatile uint32_t *base;
    struct irq irqs[MAX_IRQS];
    unsigned nregs;
};

static struct gic gic = {0}; // support only one, to make the interface simpler

static const char *irq_cfg_name(gic_irq_cfg_t t)
{
    switch (t) {
        case GIC_IRQ_CFG_LEVEL: return "LEVEL";
        case GIC_IRQ_CFG_EDGE:  return "EDGE";
        default: return "?";
   }
}

static unsigned irq_to_intid(unsigned irq, gic_irq_type_t type)
{
    switch (type) {
        case GIC_IRQ_TYPE_SPI:
                return GIC_INTERNAL + irq;
        case GIC_IRQ_TYPE_PPI:
                return GIC_NR_SGIS + irq;
        case GIC_IRQ_TYPE_SGI:
                return irq;
        default:
                panic("unknown irq type");
    }
}

static bool use_redist(gic_irq_type_t type)
{
    return (type == GIC_IRQ_TYPE_SGI || type == GIC_IRQ_TYPE_PPI) &&
        // when affinity routing is enabled, use GICR registers
        (REGB_READ32(gic.base, GICD(GICD_CTLR)) & GICD_CTRL__ARE);
}

static void check_irq(unsigned irq, gic_irq_type_t type)
{
    ASSERT(!(type == GIC_IRQ_TYPE_PPI && irq >= (GIC_INTERNAL - GIC_NR_SGIS)));
    ASSERT(!(type == GIC_IRQ_TYPE_SGI && irq >= GIC_NR_SGIS));
    ASSERT(type != GIC_IRQ_TYPE_LPI); // not supported by driver
}

// TODO: split configuration and enable into two separate functions
void gic_int_enable(unsigned irq, gic_irq_type_t type, gic_irq_cfg_t cfg) {
    // Assumes that startup code enabled EnableGrp{0,1NS,1S} in GIC_CTLR
    check_irq(irq, type);
    unsigned intid = irq_to_intid(irq, type);
    uint32_t cfg_bit = cfg == GIC_IRQ_CFG_EDGE ? 1 : 0;

    printf("GIC: enable IRQ #%u (INTID %u) type %u cfg %s\r\n",
           irq, intid, type, irq_cfg_name(cfg));

    if (use_redist(type)) {
        switch (type) {
            case GIC_IRQ_TYPE_SGI: {
                unsigned sgi_id = intid % 16;
                REGB_SET32(gic.base, GICR_PPI_SGI(GICR_ICFGR0), cfg_bit << (2 * sgi_id + 1));
                break;
            }
            case GIC_IRQ_TYPE_PPI: {
                unsigned ppi_id = intid % 16;
                REGB_SET32(gic.base, GICR_PPI_SGI(GICR_ICFGR1), cfg_bit << (2 * ppi_id + 1));
            }
            default:
                ASSERT("unreachable");
        }
        REGB_WRITE32(gic.base, GICR_PPI_SGI(GICR_ISENABLER0), 1 << (intid % 32));
    } else {
        REGB_SET32(gic.base, GICD(GICD_ICFGRn) + (intid / 16) * 4, cfg_bit << (2 * (intid % 16) + 1));
        REGB_WRITE32(gic.base, GICD(GICD_ISENABLERn) + (intid / 32) * 4, 1 << (intid % 32));
    }
}

void gic_int_disable(unsigned irq, gic_irq_type_t type) {
    check_irq(irq, type);
    unsigned intid = irq_to_intid(irq, type);

    printf("GIC: disable IRQ #%u (INTID %u) type %u\r\n",
           irq, intid, type);

    if (use_redist(type)) {
        switch (type) {
            case GIC_IRQ_TYPE_SGI:
                REGB_CLEAR32(gic.base, GICR_PPI_SGI(GICR_ICFGR0), 1 << (intid % 16));
                break;
            case GIC_IRQ_TYPE_PPI:
                REGB_CLEAR32(gic.base, GICR_PPI_SGI(GICR_ICFGR1), 1 << (intid % 16));
            default:
                ASSERT("unreachable");
        }
        REGB_WRITE32(gic.base, GICR_PPI_SGI(GICR_ICENABLER0), 1 << (intid % 32));
    } else {
        REGB_CLEAR32(gic.base, GICD(GICD_ICFGRn) + (intid / 16) * 4, 1 << (2 * (intid % 16) + 1));
        REGB_WRITE32(gic.base, GICD(GICD_ICENABLERn) + (intid / 32) * 4, 1 << (intid % 32));
    }
}

void gic_disable_all()
{
    ASSERT(false && "not implemented");
}

struct irq *gic_request(unsigned irqn, gic_irq_type_t type, gic_irq_cfg_t cfg)
{
    struct irq *irq = OBJECT_ALLOC(gic.irqs);
    irq->n = irqn;
    irq->type = type;
    irq->cfg = cfg;
    return irq;
}

void gic_release(struct irq *irq)
{
    OBJECT_FREE(irq);
}

static void gic_op_int_enable(struct irq *irq)
{
    gic_int_enable(irq->n, irq->type, irq->cfg);
}

static void gic_op_int_disable(struct irq *irq)
{
    gic_int_disable(irq->n, irq->type);
}

static void gic_op_disable_all()
{
    gic_disable_all();
}

unsigned gic_op_int_num(struct irq *irq)
{
    return irq->n;
}
unsigned gic_op_int_type(struct irq *irq)
{
    return irq->type;
}

static const struct intc_ops gic_ops = {
    .int_enable = gic_op_int_enable,
    .int_disable = gic_op_int_disable,
    .disable_all = gic_op_disable_all,
    .int_num = gic_op_int_num,
    .int_type = gic_op_int_type,
};

void gic_init(volatile uint32_t *base)
{
    uint32_t typer = REGB_READ32(base, GICD(GICD_TYPER));

    gic.base = base;
    gic.nregs = ((typer & GICD_TYPER__IT_LINES_NUMBER__MASK) >>
                                GICD_TYPER__IT_LINES_NUMBER__SHIFT) + 1;
    intc_register(&gic_ops);
    printf("GIC: base %p typer %x nregs %u\r\n", base, typer, gic.nregs);
}
