.. include:: extlinks.rst

##############################
Graphical Update Call Sequence
##############################

Call stacks for differenc graphics update scenarios.

===========================
Origin of a graphics update
===========================

Parameter change
================

.. code-block:: rst

  Control::paramChanged ()
    View::updateClient (RectRef rect)
      if window or render target of window returns shouldCollectUpdates () == true
        View::invalidate (rect)
      else View.style.isDirectUpdate () == true
        View::draw (rect)
      else
        Window::redrawView (view, rect)
          View::draw (rect)


View request
============

.. code-block:: rst

  View::invalidate () [e.g. ImageView::setProperty]


============================
Draw to a window (`Quartz`_)
============================

ImageView
=========

.. code-block:: rst

  ImageView::draw (const UpdateRgn& updateRgn)
    GraphicsPort port (this)
      View::getGraphicsDevice (offset)
        Window::getGraphicsDevice (offset)
          WindowGraphicsDevice::WindowGraphicsDevice (Window& window)
            NativeGraphicsEngine::instance ().createWindowDevice (&window)
              Window::getRenderTarget ()
   
    port.drawImage (...)
      QuartzBitmap::draw (GraphicsDevice::getNativeDevice (), ...)
        CGContextRef context = QuartzDevice::getTarget ().getContext ()
        CGContextDrawImage (context, ...)


OS trigger
==========

Trigger via View::invalidate ().

.. code-block:: rst

  QuartzOSXWindowRenderTarget::getContext ()
    QuartzOSXWindowRenderTarget::createContext ()
      NSView::lockFocusIfCanDraw
        context = [[NSGraphicsContext currentContext] graphicsPort]



Direct call
===========

Direct call of View::draw() from timer or GUI event, i.e. **not** triggered by OS.

.. code-block:: rst

  QuartzOSX/IOSWindowRenderTarget::getContext ()
    context = [[NSGraphicsContext currentContext] graphicsPort]


============================
Draw to a bitmap (`Quartz`_)
============================

Progress bar
============

.. code-block:: rst

  ProgressBarRenderer::draw
   BitmapGraphicsDevice::BitmapGraphicsDevice (Bitmap* bitmap)
    NativeGraphicsEngine::instance ().createBitmapDevice (...)
      QuartzBitmapRenderTarget::QuartzBitmapRenderTarget (quartzBitmap);
        context = CGBitmapContextCreate (...)


===================================
Draw to a layer (`Core Animation`_)
===================================

SublayerSprite
==============

.. code-block:: rst

  SublayerSprite::update
    CoreAnimationLayer::setUpdateNeeded
      [nativeLayer setNeedsDisplay]

  ... call back by OS

  drawLayer: inContext:(CGContextRef)context
    CoreAnimationLayer::draw (void* context)
      QuartzLayerRenderTarget renderTarget ((CGContextRef)context)
      QuartzScopedGraphicsDevice nativeDevice (renderTarget);
        SublayerSprite::drawLayer (IGraphics& graphics)
