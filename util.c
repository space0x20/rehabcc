int align(int nbyte, int align)
{
    return nbyte + (align - nbyte % 8);
}
