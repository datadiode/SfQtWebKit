TEMPLATE = subdirs
SUBDIRS = \
    dds \
    icns \
    jp2 \
    mng \
    tga \
    tiff \
    wbmp \
    webp

wince:SUBDIRS -= jp2 tiff

winrt {
    SUBDIRS -= tiff \
               tga
}

winrt: SUBDIRS -= webp
