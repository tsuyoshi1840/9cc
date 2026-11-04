.intel_syntax noprefix
.data
.LC0:
  .string "Hello from file!\n"
.text
.globl main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 0
  lea rax, [rip+.LC0]
  push rax
  pop rdi
  mov rax, rsp
  and rax, 15
  jnz .Lcall0
  mov rax, 0
  call printf
  jmp .Lend0
.Lcall0:
  sub rsp, 8
  mov rax, 0
  call printf
  add rsp, 8
.Lend0:
  push rax
  pop rax
  push 0
  pop rax
  mov rsp, rbp
  pop rbp
  ret
  mov rsp, rbp
  pop rbp
  ret
