;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;                                                               ;
;              Interrupt service routines wrappers              ;
;                                                               ;
;    Provides wrappers for the ISRs.                            ;
;                                                               ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Global functions ;;

global isr_default_exc
global isr_pf_exc
global isr_default_pic1_irq
global isr_default_pic2_irq
global isr_pit_irq
global isr_kbd_irq
global isr_0x09_irq
global isr_0x0a_irq
global isr_0x0b_irq
global isr_syscall
global isr_yield

;; External functions ;;

extern _isr_default_exc
extern _isr_pf_exc
extern _isr_default_pic1_irq
extern _isr_default_pic2_irq
extern _isr_pit_irq
extern _isr_kbd_irq
extern _isr_0x09_irq
extern _isr_0x0a_irq
extern _isr_0x0b_irq
extern _isr_syscall
extern _isr_yield

;; Macros for saving/restoring registers ;;

%macro SAVE_REGISTERS 0
	pushad
	push ds
	push es
	push fs
	push gs
%endmacro

%macro RESTORE_REGISTERS 0
	pop gs
	pop fs
	pop es
	pop ds
	popad
%endmacro

;; EOI macros ;;

%macro EOI_MASTER 0
	mov al, 0x20
	out 0x20, al
%endmacro

%macro EOI_SLAVE 0
	mov al, 0x20
	out 0xa0, al
%endmacro

;; Exceptions ;;

isr_default_exc:
	cli
	SAVE_REGISTERS
	call _isr_default_exc
	RESTORE_REGISTERS
	add esp, 4
	iret

isr_pf_exc:
	cli
	SAVE_REGISTERS
	call _isr_pf_exc
	RESTORE_REGISTERS
	add esp, 4
	iret

;; IRQs ;;

isr_default_pic1_irq:
	SAVE_REGISTERS
	call _isr_default_pic1_irq
	EOI_MASTER
	RESTORE_REGISTERS
	iret

isr_default_pic2_irq:
	SAVE_REGISTERS
	call _isr_default_pic2_irq
	EOI_SLAVE
	EOI_MASTER
	RESTORE_REGISTERS
	iret

isr_pit_irq:
	SAVE_REGISTERS
	call _isr_pit_irq
	EOI_MASTER
	RESTORE_REGISTERS
	iret

isr_kbd_irq:
	SAVE_REGISTERS
	call _isr_kbd_irq
	EOI_MASTER
	RESTORE_REGISTERS
	iret

isr_0x09_irq:
	SAVE_REGISTERS
	call _isr_0x09_irq
	EOI_SLAVE
	EOI_MASTER
	RESTORE_REGISTERS
	iret

isr_0x0a_irq:
	SAVE_REGISTERS
	call _isr_0x0a_irq
	EOI_SLAVE
	EOI_MASTER
	RESTORE_REGISTERS
	iret

isr_0x0b_irq:
	SAVE_REGISTERS
	call _isr_0x0b_irq
	EOI_SLAVE
	EOI_MASTER
	RESTORE_REGISTERS
	iret

;; System call ;;

isr_syscall:
	SAVE_REGISTERS
	call _isr_syscall
	RESTORE_REGISTERS
	iret

;; Yield hack ;;

isr_yield:
	cli
	SAVE_REGISTERS
	call _isr_yield
	RESTORE_REGISTERS
	iret
