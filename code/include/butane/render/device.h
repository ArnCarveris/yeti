// This file is part of Butane. See README.md and LICENSE.md for details.
// Copyright (c) 2012 Michael Williams <devbug@bitbyte.ca>

#ifndef _BUTANE_RENDER_DEVICE_H_
#define _BUTANE_RENDER_DEVICE_H_

#include <butane/foundation.h>
#include <butane/config.h>

namespace butane {
  class BUTANE_EXPORT RenderDevice abstract {
    __foundation_trait(RenderDevice, non_copyable);

    protected:
      RenderDevice();
      virtual ~RenderDevice();

    public:
      virtual void dispatch() = 0;
  };
} // butane

#endif // _BUTANE_RENDER_DEVICE_H_
