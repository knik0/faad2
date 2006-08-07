if HAVE_MPEG4IP_PLUG
if HAVE_XMMS
SUBDIRS = xmms mpeg4ip
else
SUBDIRS = mpeg4ip
endif #HAVE_XMMS
else
if HAVE_XMMS
SUBDIRS = xmms
else
if HAVE_BMP
SUBDIRS = bmp
else
SUBDIRS =
endif #HAVE_BMP
endif #HAVE_XMMS
endif #HAVE_MPEG4IP_PLUG
