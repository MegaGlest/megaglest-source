;
;  Copyright (c) 2010 The VP8 project authors. All Rights Reserved.
;
;  Use of this source code is governed by a BSD-style license 
;  that can be found in the LICENSE file in the root of the source
;  tree. An additional intellectual property rights grant can be found
;  in the file PATENTS.  All contributing project authors may 
;  be found in the AUTHORS file in the root of the source tree.
;

_text SEGMENT
; %ifidn __OUTPUT_FORMAT__,x64
; %ifdef _WIN64

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

end