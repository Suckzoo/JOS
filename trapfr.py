regs = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15']


# for r in regs:
#     asm = f'"mov %c[{r}](%0), %%{r}\\n\\t"'
#     print(asm)
for r in regs:
    asm = f'"mov %%{r}, %c[{r}](%0)\\n\\t"'
    print(asm)
