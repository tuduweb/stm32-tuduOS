	AREA |.text|, CODE, READONLY
	;IMPORT lwt_set_kernel_sp
	
	EXPORT syscall
syscall;���±���R0~R7����
	PUSH {R4-R7}
	MOV R7, R0

    ;MOV     R0, SP              ; v1 = SP
    ;BL      lwt_set_kernel_sp   ; lwp_set_kernel_sp(v1) ;sp�Ĵ���ָ��stack,���浱ǰ(kernel)SP��ֵ

	MOV R0, R1
	MOV R1, R2
	MOV R2, R3
	ADD R6, SP, #0x10
	LDM R6, {R3-R5};�����ݼ���,����ַ�ϵ�ֵ���ص��Ĵ�����
	SVC 0
	POP {R4-R7}
	BX  LR
	


;/*
; * void lwp_user_entry(args, text, data);
; */
	IMPORT lwt_set_kernel_sp
	EXPORT lwp_save_sp
lwp_save_sp
    MOV     R0, SP              ; v1 = SP
	MOV		R8,	LR
    BL      lwt_set_kernel_sp   ; lwp_set_kernel_sp(v1) ;sp�Ĵ���ָ��stack,���浱ǰ(kernel)SP��ֵ
	MOV		LR,	R8
	BX		LR
    END
