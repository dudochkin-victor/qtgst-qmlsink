#include <QtGui>
#include <QtDeclarative>
#include <QtOpenGL/QGLWidget>
#include <gst/gst.h>
#include <QtGstQmlSink/qvideosurfacegstsink.h>
#include <QtGstQmlSink/gstvideoitem.h>

typedef struct pad_added_args {
  GstBin *pipeline;
  GstElement *video;
  GstElement *audio;
} PadAddedArgs;

void pad_added_cb(GstElement *src, GstPad *pad, pad_added_args * args)
{
  GstPad *tpad = gst_element_get_compatible_pad(args->video, pad, NULL);
  if (tpad) {
    gst_pad_link(pad, tpad);
    return;
  }

  tpad = gst_element_get_compatible_pad(args->audio, pad, NULL);
  if (tpad) {
    gst_pad_link(pad, tpad);
    return;
  }
}

void
makeSource (int argc, char **argv, PadAddedArgs *args) {
  
  if (argc >= 2) {
    GstElement *src = gst_element_factory_make("uridecodebin", "src");
    gst_bin_add(args->pipeline, src);
    g_object_set(G_OBJECT(src), "uri", argv[1], NULL);
    g_signal_connect(G_OBJECT(src), "pad-added", G_CALLBACK(pad_added_cb), args);
  } else {
    GstElement *vsrc = gst_element_factory_make("videotestsrc", NULL);

    g_object_set(G_OBJECT(vsrc), "is-live", TRUE, NULL);
    
    GstElement *vcapsflt = gst_element_factory_make("capsfilter", NULL);

    g_object_set(G_OBJECT(vcapsflt), "caps",
		 gst_caps_from_string("video/x-raw-yuv,"
				      "width=(int)1280,"
				      "framerate=(fraction) 30/1,"
				      "height=(int)720"), NULL);
								  
    GstElement *asrc = gst_element_factory_make("audiotestsrc", NULL);
    gst_bin_add_many (args->pipeline, vsrc, vcapsflt, asrc, NULL);
    gst_element_link_many(vsrc, vcapsflt, args->video, NULL);
    gst_element_link(asrc, args->audio);
  }
}

typedef struct _makePipeline {
  int argc;
  char **argv;
  QPainterVideoSurface *surface;
} MakePipelineArgs;

gboolean makePipeline(void *user)
{
  MakePipelineArgs *args = (MakePipelineArgs *) user;

  GstElement *p = gst_pipeline_new("pipeline");
  GstElement *vsnk = GST_ELEMENT(QVideoSurfaceGstSink::createSink(args->surface));
  GstElement *vq = gst_element_factory_make("queue2", NULL);
  GstElement *aq = gst_element_factory_make("queue2", NULL);
  GstElement *asnk = gst_element_factory_make("autoaudiosink", "fakesink");

  PadAddedArgs my_args = { GST_BIN(p), vq, aq };

  gst_bin_add_many(GST_BIN(p), vsnk, vq, aq, asnk, NULL);
  gst_element_link_many(vq, vsnk, NULL);
  gst_element_link_many(aq, asnk, NULL);
  makeSource(args->argc, args->argv, &my_args);
  
  gst_element_set_state(p, GST_STATE_PLAYING);

  return FALSE;
}

int main (int argc, char **argv)
{
  QApplication app(argc, argv);
  gst_init(&argc, &argv);

  /* register our custom types with qml */
  qmlRegisterType<GstVideoItem>("Gst", 1, 0, "VideoItem");
  qmlRegisterType<QPainterVideoSurface>("Gst", 1, 0, "VideoSurface");

  QDeclarativeView v;
  QGLWidget *g = new QGLWidget;
  v.setViewport(g);
  v.show();
  v.setResizeMode(QDeclarativeView::SizeRootObjectToView);
  v.resize(320, 240);

  // both the video sink and the video item need access to a
  // QPainterVideoSurface, which does accelerated drawing of video
  // frames
  QPainterVideoSurface *surface = new QPainterVideoSurface;
  surface->setGLContext((QGLContext *) g->context());

  // make the video surface available in QML space
  // a "VideoItem" created in QML can now use this as its "surface" property
  v.rootContext()->setContextProperty("videosurface", surface);
  
  // load QML resources only after the videosurface is available. this
  // suppresses some warnings from the QML runtime
  v.setSource(QUrl::fromLocalFile("test.qml"));

  MakePipelineArgs args = { argc, argv, surface };
  makePipeline(&args);

  app.exec();
  return 0;
}
