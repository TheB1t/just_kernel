#include <proc/syscalls.h>
#include <int/isr.h>
#include <sys/smp.h>

void* syscalls[256] = { 0 };

void syscalls_set(uint32_t num, void* call) {
	if (num >= 256)
		return;

	syscalls[num] = call;
}

void syscalls_handler(core_locals_t* locals) {
	void* location = syscalls[locals->irq_regs->base.eax];

	if (!location)
		return;

	asm volatile ("		\
		push %1;		\
		push %2;		\
		push %3;		\
		push %4;		\
		push %5;		\
		call *%6;		\
		pop %%ebx;		\
		pop %%ebx;		\
		pop %%ebx;		\
		pop %%ebx;		\
		pop %%ebx;		"
		:	"=a" (locals->irq_regs->base.eax)
		:	"r" (locals->irq_regs->base.edi),
			"r" (locals->irq_regs->base.esi),
			"r" (locals->irq_regs->base.edx),
			"r" (locals->irq_regs->base.ecx),
			"r" (locals->irq_regs->base.ebx),
			"r" (location)
	);
}

void syscalls_init() {
	register_int_handler(0x80, syscalls_handler);
}