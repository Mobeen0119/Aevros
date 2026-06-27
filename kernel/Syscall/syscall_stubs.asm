global write
global read 
global open
global close
global fork
global exit

write:
     mov eax,1
     mov ebx , [esp + 4]
     mov ecx , [esp + 8]
     mov edx , [esp + 12]
     int 0x80
     ret

read:
    mov eax,2
    mov ebx , [esp + 4]
    mov ecx , [esp + 8]
    mov edx , [esp + 12]
    int 0x80
    ret

open:
    mov eax,3
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    int 0x80
    ret

close:
    mov eax,4
    mov ebx, [esp + 4]
    int 0x80
    ret

fork:
    mov eax,5
    int 0x80   
    ret

exit:
    mov eax,6
    mov ebx, [esp + 4]
    int 0x80
    ret