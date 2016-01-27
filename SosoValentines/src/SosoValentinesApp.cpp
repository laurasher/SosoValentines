#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/svg/Svg.h"
#include "cinder/Timeline.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class SosoValentinesApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

	gl::TextureRef	 texture;
	gl::TextureFontRef aFont;
};

void SosoValentinesApp::setup()
{
	 texture = gl::Texture::create( loadImage(getAssetPath("arrow_heart.jpg")));
}

void SosoValentinesApp::mouseDown( MouseEvent event )
{
}

void SosoValentinesApp::update()
{
}

void SosoValentinesApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::draw(texture);
}

CINDER_APP( SosoValentinesApp, RendererGl )
