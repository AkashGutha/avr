int main(void)
{
    while (1)
    {
        __asm__ volatile("nop");		// so the endless loop isn't optimized away
    }

    return (1);	// should never happen
}
