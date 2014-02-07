;
;  Copyright (c) 2014 MegaGlest. All Rights Reserved.
;
;  Use of this source code is governed by the GPL 3.x license 

_text SEGMENT
; %ifidn __OUTPUT_FORMAT__,x64
; %ifdef _WIN64
IFDEF RAX

streflop_winx64_fclex PROC FRAME

	fclex
	.endprolog
	ret
streflop_winx64_fclex ENDP

streflop_winx64_fldcw PROC FRAME

	fclex
;    mov qword ptr [rsp + 8], rcx
;    .ENDPROLOG
;    fldcw [rsp + 8]
;    ret
	sub   rsp, 8
    mov   [rsp], rcx ; win x64 specific
	.ENDPROLOG
    fldcw [rsp]
    add   rsp, 8
    ret
streflop_winx64_fldcw ENDP

streflop_winx64_fstcw PROC FRAME

    sub rsp, 8
    .ENDPROLOG
    fstcw [rsp]
    mov rax, [rsp]
    add rsp, 8
    ret
streflop_winx64_fstcw ENDP

streflop_winx64_stmxcsr PROC FRAME

    sub rsp, 8
    .ENDPROLOG
    stmxcsr [rsp]
    mov rax, [rsp]
    add rsp, 8
    ret
streflop_winx64_stmxcsr ENDP

streflop_winx64_ldmxcsr PROC FRAME

    sub rsp, 8
    .ENDPROLOG
    ldmxcsr [rsp]
    mov rax, [rsp]
    add rsp, 8
    ret
streflop_winx64_ldmxcsr ENDP

;%endif
ENDIF

end