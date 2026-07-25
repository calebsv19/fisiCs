ENTRY(_start)

SECTIONS
{
  . = 0x00010000;

  .text : ALIGN(16)
  {
    KEEP(*(.text.entry))
    *(.text .text.*)
  }

  .rodata : ALIGN(16)
  {
    *(.rodata .rodata.*)
  }

  .data : ALIGN(16)
  {
    *(.data .data.*)
  }

  .bss : ALIGN(16)
  {
    *(COMMON)
    *(.bss .bss.*)
  }

  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
  }
}
