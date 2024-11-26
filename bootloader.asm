; bootloader.asm - A simple bootloader
BITS 16                  ; We are in 16-bit mode
ORG 0x7C00               ; The bootloader will be loaded at address 0x7C00

start:
    ; Clear the screen by setting all video memory to spaces
    mov ax, 0xB800       ; Base address of video memory in real mode
    mov es, ax
    xor di, di           ; Offset in video memory
    mov ah, 0x0F         ; White text on black background
    mov al, ' '          ; ASCII space character
    mov cx, 2000         ; Clear 2000 characters (entire screen)
    rep stosw            ; Fill the video memory with spaces

    ; Print "Hello, World!" to the screen
    mov si, hello_msg    ; Load the address of the string
    call print_string    ; Call the function to print the string

    ; Infinite loop to halt the system
    jmp $

; Function to print a null-terminated string
print_string:
    mov ah, 0x0E         ; BIOS teletype function (print character)
next_char:
    lodsb                ; Load next byte from string into AL
    cmp al, 0            ; Check for null terminator
    je done_printing     ; If null, we're done
    int 0x10             ; Call BIOS interrupt to print character
    jmp next_char        ; Repeat for the next character
done_printing:
    ret

hello_msg db 'Hello, Dorld!', 0

; Boot signature - must be at the end of the boot sector
times 510 - ($ - $$) db 0 ; Pad the rest of the 512 bytes with zeros
dw 0xAA55                 ; Boot sector signature
