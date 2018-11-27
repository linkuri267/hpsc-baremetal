#include <stdint.h>

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
#define GICR_ICFGR0             0x0c00
#define GICR_ICFGR1             0x0c04
#define GICR_IGROUPMODR0        0x0d00

#define GICD_CTRL__ARE          (1 << 4) // when no security extention, use only ARE_S bit
#define GICD_CTRL__ARE_S        (1 << 4)
#define GICD_CTRL__ARE_NS       (1 << 5)

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

static void get_int_regs(unsigned intid, gic_irq_type_t type,
                         uint32_t *reg_enable, unsigned *shift_enable,
                         uint32_t *reg_cfg, unsigned *shift_cfg)
{
    if ((type == GIC_IRQ_TYPE_SGI || type == GIC_IRQ_TYPE_PPI) &&
        // when affinity routing is enabled, use GICR registers
        (REGB_READ32(gic.base, GICD(GICD_CTLR)) & GICD_CTRL__ARE)) {
        *reg_enable = GICR_PPI_SGI(GICR_ISENABLER0);
        switch (type) {
            case GIC_IRQ_TYPE_SGI: *reg_cfg = GICR_PPI_SGI(GICR_ICFGR0); break;
            case GIC_IRQ_TYPE_PPI: *reg_cfg = GICR_PPI_SGI(GICR_ICFGR1); break;
            default: ASSERT("unreachable");
        }
        *shift_enable = intid % 32;
        *shift_cfg    = intid % 16;
    } else {
        *reg_enable = GICD(GICD_ISENABLERn) + (intid / 32) * 4;
        *shift_enable = intid % 32;

        *reg_cfg = GICD(GICD_ICFGRn) + (intid / 16) * 4;
        *shift_cfg = intid % 16;
    }
}

static void check_irq(unsigned irq, gic_irq_type_t type)
{
    ASSERT(!(type == GIC_IRQ_TYPE_PPI && irq >= (GIC_INTERNAL - GIC_NR_SGIS)));
    ASSERT(!(type == GIC_IRQ_TYPE_SGI && irq >= GIC_NR_SGIS));
    ASSERT(type != GIC_IRQ_TYPE_LPI); // not supported by driver
}

void gic_int_enable(unsigned irq, gic_irq_type_t type, gic_irq_cfg_t cfg) {
    // Assumes that startup code enabled EnableGrp{0,1NS,1S} in GIC_CTLR

    uint32_t reg_enable, reg_cfg;
    unsigned shift_enable, shift_cfg;
    check_irq(irq, type);

    unsigned intid = irq_to_intid(irq, type);
    get_int_regs(intid, type, &reg_enable, &shift_enable, &reg_cfg, &shift_cfg);

    printf("GIC: enable IRQ #%u (INTID %u) type %u cfg %s\r\n",
           irq, intid, type, irq_cfg_name(type));
    REGB_SET32(gic.base, reg_enable, 1 << shift_enable);

    printf("GIC: configure IRQ #%u (INTID %u)\r\n", irq, intid);
    REGB_SET32(gic.base, reg_cfg,
               (cfg == GIC_IRQ_CFG_EDGE ? 1 : 0) << shift_cfg);
}

void gic_int_disable(unsigned irq, gic_irq_type_t type) {
    uint32_t reg_enable, reg_cfg;
    unsigned shift_enable, shift_cfg;
    check_irq(irq, type);

    unsigned intid = irq_to_intid(irq, type);
    get_int_regs(intid, type, &reg_enable, &shift_enable, &reg_cfg, &shift_cfg);

    printf("GIC: disable IRQ #%u (INTID %u)\r\n", irq, intid);
    REGB_CLEAR32(gic.base, reg_enable, 1 << shift_enable);
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
    .int_num = gic_op_int_num,
    .int_type = gic_op_int_type,
};

void gic_init(volatile uint32_t *base)
{
    gic.base = base;
    intc_register(&gic_ops);
}
