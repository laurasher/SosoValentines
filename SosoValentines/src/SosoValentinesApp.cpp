#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "cinder/Utilities.h"
#include "CinderImGui.h"

#include "TrianglePiece.h"
#include "InstagramStream.h"
#include "TextRibbon.h"

using namespace ci;
using namespace ci::app;
using namespace std;

static const int MIRROR_DUR = 5;	// Duration of the mirror/kaleidoscope animation
static const int STILL_DUR = 2;		// Duration of the still image
static const string TAG = "";		// Instagram tag to search for

// Instagram Client Id - DO NOT USE THIS ONE!!! 
// Replace with your own client ID after registering your
// instagram application here http://instagram.com/developer/register/
static const string CLIENT_ID = "def20410b5134f7d9b828668775aee4a";

static const bool PREMULT = false;

class SosoValentinesApp : public App {
private:
	void	setup();
	void	update();
	void	draw();
	
	void	updateMirrors(vector<TrianglePiece> *vec);
	void	drawMirrors(vector<TrianglePiece> *vec);
	void	checkImageLoaded();
	void	defineMirrorGrid();
	void	transitionMirrorIn(vector<TrianglePiece> *vec);
	
	void	changePhase(int newPhase);
	void	resetSample();
	void	newInstagram();
	void	continueCycle();
	void	mirrorOut();
	void	mirrorIn();
	void	imageLoaded();
	void	mouseDown(MouseEvent event);
	void	mouseDrag(MouseEvent event);
	bool  inTriangleCheck(ci::vec2 mVertices[3], ci::vec2 mousePos);
	
	gl::TextureRef					mNewTex;				// the loaded texture
	gl::TextureRef					mBgTexture;			// texture for the still image
	gl::TextureRef					mMirrorTexture;	// texture for the mirror
	gl::TextureRef					mHeartTexture;  // texture for the heart cutout
	
	vector<TrianglePiece>		mTriPieces;				// stores all of the kaleidoscope mirror pieces
	Anim<vec2>							mSamplePt;				// location of the piece of the image that is being sampled for the kaleidoscope
	int											mPhase;						// current phase of the app (0 or 1)
	Instagram								mCurInstagram;		// current instagram info
	
	shared_ptr<InstagramStream> mInstaStream;			// stream and loader for instagram data
	bool												mLoadingTexture;	// If the texture image is currently loading
	bool												mTextureLoaded;		// If the new texture has finished loading
	float												mSampleSize;			// Size of the image sample to grab for the kaleidoscope
	TextRibbon									*mTextRibbon;			// The text ribbon that shows up in "sill image" mode
	Anim<float>									mMirrorRot;				// rotation of the canvas in kaleidoscope mode
	bool												mPiecesIn;				// whether all of the mirror pieces are showing or not
	bool												mPhaseChangeCalled;		// if the app has been told to change phases or not
  bool                        mDrawingMirrorTex;  // boolean used to determine when to call updateMirrors
  bool                        mFirstRun;				// if the app is on its first cycle
		// helpful for debug
	bool                        isInDebugMode = true;
	bool                        isDrawingHeartCutout;   // turn heart cutout on/off
	bool                        isDrawingOriginalImage; // show original image
	bool												isDrawingOneHexagon ;	// show just the first hexagon in 1 * 1 grid texture rectangle
	bool												isDisablingGlobalRotation;
	bool												isRandomizingHexInitalization;	// this affects the initial size, position
	bool												isRotatingHexagon;							//	rotate the hexagon 30 degrees. Need to change the triangle coordinates, scale (reflection), grid, and alpha
	bool												isUsingBoxTexture;							//use the diamond box opacity texture
	bool												isMousing;
	int													tri_index;											// which of the triangles to show of the first hexagon
	int													nthHexagon;
	TextLayout									layout;

	vec2 start;

	// mouse vars
	vec2 mousePos;
	std::vector<vec2> mousePoints;

};

void SosoValentinesApp::setup()
{
	setWindowSize( 1080, 1080 ); // 1080 x 1080 is Instagram's current resolution
	setFrameRate( 60 );

	mPhase = 1;
	mFirstRun = true;
	mLoadingTexture = false;
	mTextureLoaded = false;
	mPhaseChangeCalled = false;
  mDrawingMirrorTex = false;
	tri_index = 0;
	nthHexagon = 0;

	if(isInDebugMode){
		ui::initialize();
		isDrawingHeartCutout = false;
		isDrawingOriginalImage = true;
		isDrawingOneHexagon = false;
		isDisablingGlobalRotation = true;
		isRandomizingHexInitalization = false;
		isRotatingHexagon = true;
		isUsingBoxTexture = true;
		isTwinklingWithOpacity = true;
		isMousing = true;

	} else {
		isDrawingHeartCutout = true;
		isDrawingOriginalImage = true;
		isDrawingOneHexagon = false;
		isDisablingGlobalRotation = false;
		isRandomizingHexInitalization = true;
		isRotatingHexagon = true;
		isUsingBoxTexture = true;
		isTwinklingWithOpacity = true;
		isMousing = false;
	}

	auto heartCutout = loadImage( loadAsset( "heart_cutout_18.png" ) );
	mHeartTexture = gl::Texture2d::create( heartCutout );

	mTextRibbon = new TextRibbon();

	// Popular images stream
	//mInstaStream = make_shared<InstagramStream>( CLIENT_ID );
	// Image stream of a particular tag
//	mInstaStream = make_shared<InstagramStream>( "sosolimited", CLIENT_ID );
	mInstaStream = make_shared<InstagramStream>( "sweetheartscandy", CLIENT_ID );// Image stream in a particular area
	// mInstaStream = make_shared<InstagramStream>( vec2(40.720467,-74.00603), 5000, CLIENT_ID );
    
	continueCycle();

}

// This is where the new cycle should be started.
// A cycle consists of an instagram loading, the kaleidoscope animating for a few seconds, 
// transitioning out and landing on the still image for a few seconds. 
// This happens once a cycle is complete, meaning that the mirrors have transitioned out.
void SosoValentinesApp::continueCycle()
{
	mPhaseChangeCalled = false;		// it has net been told to change phases yet. It should only do that once shit is loaded
	defineMirrorGrid();						// redefine the kaleidoscope grid
	mTextureLoaded = false;				// This will trigger checking if an image has loaded in the update function
	newInstagram();								// grab the next instagram item
	mDrawingMirrorTex = false;		// reset the mirror drawing
}

// Creates the grid of kaleidoscope mirrored triangles
void SosoValentinesApp::defineMirrorGrid()
{
	const int r = 1; // don't change this because this normalizes each triangle
	const int numTriangles = 18;	// 50 to match heart_cutout_50.png. When doing so make sure to change to float startY = (tri_width * 1.5 * j) - (tri_width /2);

	// this controls the initial position of the kaleidoscope
	float tri_scale = 0.0f;

	if (isRandomizingHexInitalization){
		tri_scale = (float)randInt(50, 100);
	}
	else {
		if (!isRotatingHexagon) {
			tri_scale = ((float)getWindowWidth()) / ((float)(numTriangles));
		} else {
			tri_scale = ((float)getWindowWidth()) / ((float)(numTriangles * r * sin(M_PI / 3)));
		}
	}
    
	// delete any previous pieces and clear the old vector
	mTriPieces.clear();
	
	vec2 pt1(0.0, 0.0);
	vec2 pt2(0.0, 0.0);
	vec2 pt3(0.0, 0.0);

	if( !isRotatingHexagon ) {
		pt1 = vec2(0.0, 0.0);
		pt2 = vec2(r, 0.0);
		pt3 = vec2((cos(M_PI/3) * r), (sin(M_PI/3) * r));
	} else {
		pt1 = vec2(0.0, 0.0);
		pt2 = vec2((sin(M_PI/3) * r),(cos(M_PI/3) * r));
		pt3 = vec2((sin(M_PI/3) * r),(cos(M_PI/3) * r * (-1)));
	}

	const float tri_width = distance( pt1, pt2 ) * tri_scale;
	const float tri_height = std::sqrt((tri_width*tri_width) - ((tri_width/2) * (tri_width/2)));
    // cout << tri_height *2;
	
    // amtX and amtY controls the circling texture over the original image
	//	int	amtX = ceil((((getWindowWidth()*2) - .5) / (1.5*(tri_width))) + 0.5f );
	//	int amtY = ceil((getWindowHeight()*2) / (tri_height) + 0.5f );
	float diagonal = getWindowWidth(); //make this =1. If we will be rotating+ shifting the grid, apply a factor > 1
	//float diagonal = sqrt((getWindowWidth() * getWindowWidth()) + (getWindowHeight() * getWindowHeight()));
	int amtX;
	int amtY;

	if (!isDisablingGlobalRotation) { //need those extra texures and we should set the rotation to start from the center
		amtX = numTriangles * (diagonal/getWindowWidth()); // draw some extra so we have enough for when it's rotating
		amtY = (int)(numTriangles * (float)(getWindowHeight())/(float)(getWindowWidth()) * (diagonal/getWindowWidth()));
	} else {
		amtX = numTriangles / 2 + 4; // need to divide by 2 because hex width = 2 * triangles. add some extra to fill the whole page
		amtY = numTriangles / 2 + 4; // because it's a square
	}

	// set the coordinate back to have top left corner as the origin
	const float xOffset = (-1) * getWindowWidth()/2;
	const float yOffset = (-1) * getWindowHeight()/2 - (tri_width/2);

	// creates a series of hexagons composed of 6 triangles each
	if (!isRotatingHexagon) {
		for( int i = 0; i <= amtX; i++ ) {
			float startX = ((tri_width) * 1.5 * i);
			startX += xOffset;
			for( int j = 0; j <= amtY; j++ ) {
				float startY = (i%2==0) ? (tri_height*2*j) - (tri_height) : tri_height*2*j;
				startY += yOffset;
				
				for( int k = 0; k < 6; k++ ) {
					// because every other pieces is a mirror of the one next to it, every other has to be reversed on the x scale
					int scaleX = (k%2==0) ? 1 : -1;

					vec2 scale = vec2( scaleX * tri_scale, tri_scale );
					start = vec2(startX, startY );
					
					// assign transparency for the sides of the cube
					float alpha = 0.0f;
					if (!isUsingBoxTexture) {
						alpha = 1.0f;
					}
					else {
						if (k == 0 || k == 1) {alpha = 0.4f;}
						else if (k == 2 || k == 5) {alpha = 0.7f;}
						else {alpha = 1.0f;}
					}

					// rotate the whole triangle -120 degrees CC so the hexagon will have the the vertex at top
					// the scale flips the location of the odd number triangles
					TrianglePiece tri = TrianglePiece(vec2(startX, startY), pt1, pt2, pt3, M_PI / 3 * k, scale, alpha);
					mTriPieces.push_back(tri);
				}
			}
		}
	} else {
		// hex vertex at the top
		for( int j = 0; j < amtY; j++ ) {
			//float startY = (tri_width * 1.5 * j) - (tri_width /2);	// shift tri_width/2 up so that the top of the first cube matches the top
			float startY = (tri_width * 1.5 * j);
			startY += yOffset;
			for( int i = 0; i < amtX; i++ ) {
				float startX = (j%2==0) ? (tri_height*2*i) : (tri_height*2*i - tri_height);
				startX += xOffset;

				for( int k = 0; k < 6; k++ ) {
					// because every other pieces is a mirror of the one next to it, every other has to be reversed on the x scale
					int scaleX = (k%2==0) ? 1 : -1;

					vec2 scale = vec2( tri_scale, scaleX * tri_scale );
					start = vec2(startX, startY );
					// assign transparency for the sides of the cube
					float alpha = 0.0f;
					if (!isUsingBoxTexture) {
						alpha = 1.0f;
					}
					else {
						if (k == 2 || k == 3) {alpha = 0.4f;}
						else if (k == 1 || k == 4) {alpha = 0.7f;}
						else {alpha = 1.0f;}
					}
					// rotate the whole triangle -120 degrees CC so the hexagon will have the the vertex at top
					// the scale flips the location of the odd number triangles
					TrianglePiece tri = TrianglePiece(vec2(startX, startY), pt1, pt2, pt3, M_PI / 3 * k, scale, alpha);
					mTriPieces.push_back(tri);
				}
			}
		}
	}
}

// replaces the mCurInstagram with new Instagram
void SosoValentinesApp::newInstagram()
{
	if( mInstaStream->hasInstagramAvailable() )
		mCurInstagram = mInstaStream->getNextInstagram();
}

void SosoValentinesApp::changePhase( int newPhase )
{	
	mPhase = newPhase;
	
	switch( mPhase ) {
		// Mirror Mode
		case 0: {
      mDrawingMirrorTex = true;
      // transition all of the mirror pieces in
      
	// if mousepos and drag does not equal 0, begin trans in
	//		if ( mousePos.x!=0 && mousePoints.size()!=0 )
	/*if(isMousing){
				if ( mousePos.x!=0 ){
					vec2 mousePosAdd;
					mousePosAdd.x = mousePos.x+20; mousePosAdd.y = mousePos.y+20;

					transitionMirrorIn( &mTriPieces );

					console()<<mousePoints.size()<<endl;
					console()<<mousePos<<endl;
					gl::color( Color::white() );
					gl::drawLine(mousePos, mousePos);
				}
			} else{
*/
	cout << "switched to mirror mode at " <<timeline().getCurrentTime() << "s" << endl;

      // rotation during the mirror mode
			mMirrorRot = randFloat(M_PI, M_PI * 2);
			float newRot = mMirrorRot + randFloat(M_PI, M_PI/4);
			timeline().apply(&mMirrorRot, newRot, MIRROR_DUR, EaseInOutQuad());
			mPiecesIn = false;

    }
		break;
		// Still Image Mode
    case 1:
			if (mFirstRun) {
				mFirstRun = false;
			}
			mDrawingMirrorTex = false;
			// transition all of the mirror pieces out
			// setTransitionOut (setTransition) has timeline too inside TrianglePiece.cpp
			cout << "switched to still image mode at " <<timeline().getCurrentTime() << "s" << endl;
      
			// start the transitioning process before swi
			transitionMirrorIn( &mTriPieces );
			resetSample();
/*
			if(isMousing){
			// set mouse linked sample size and position back to 0
			mousePos.x = 0; mousePos.y = 0; mousePoints.clear();
			}
*/

// set mouse linked sample size and position back to 0

		break;
	}
}

void SosoValentinesApp::checkImageLoaded()
{
	mLoadingTexture = true;
	mTextureLoaded = false;
	
	gl::TextureRef tex = gl::Texture::create( mCurInstagram.getImage() );
	if( ! mCurInstagram.getImage().getData() )
		return;
	
	// THE IMAGE HAS BEEN LOADED 
	mLoadingTexture = false;
	mTextureLoaded = true;
	cout << "image loaded" << endl;

	// assign the new textures
	mNewTex = tex;
	mBgTexture = mNewTex;
	mMirrorTexture = mNewTex;
}

void SosoValentinesApp::transitionMirrorIn( vector<TrianglePiece> *vec )
{
	cout << "transitionMirrorIn" << endl;
	for( int i = 0; i < vec->size(); i++ ) {
		//float delay = randFloat( 0.1f, 0.5f );
    float delay = 0.5f;
    
    (*vec)[i].reset( delay, mMirrorTexture );
		(*vec)[i].hasTransitionedIn = true;
	}

  mTextRibbon->ribbonOut(0);
	//mTextRibbon->ribbonIn(0);
  //cout << "drawing ribbon" << endl;
}

void SosoValentinesApp::imageLoaded()
{
	mPhaseChangeCalled = true;
	// we don't want to wait on the first go around
	int delayOffset = MIRROR_DUR;
	if( mFirstRun ) {
		// mFirstRun = false;
		delayOffset = 0;
	}

	// This defines the length of time that we're in each phase
	timeline().add( [this] { changePhase(1); }, timeline().getCurrentTime() + delayOffset );	// still mode first
	timeline().add( [this] { changePhase(0); }, timeline().getCurrentTime() + delayOffset + STILL_DUR ); // then mirror mode after STILL_DUR
}

void SosoValentinesApp::resetSample()
{
	if(isMousing){
			mSampleSize = randInt(100, 300);
			mSamplePt.value().y = mousePos.y;
			mSamplePt.value().x = mousePos.x;
		return;
	}
	// reset sample pos
	mSampleSize = randInt(100, 300);
	mSamplePt.value().y = randFloat(0, getWindowWidth() - mSampleSize);
	mSamplePt.value().x = randFloat(0, getWindowHeight() - mSampleSize);


	vec2 newPos;
	int count = 0;
	// Try to find a good sample location thats within the window's frame.
	// Give up if we try and settle after 150 times, no big deal.
	do {
		newPos.x = randFloat(0, getWindowWidth() - mSampleSize/2);
		newPos.y = randFloat(0, getWindowHeight() - mSampleSize/2);
		count++;
	} while(count < 150	&& ((mSamplePt.value().x - newPos.x) < 100 || (mSamplePt.value().y - newPos.y) < 100));
	timeline().apply(&mSamplePt, newPos, STILL_DUR - 1, EaseInOutQuad()).delay(.5);
}

void SosoValentinesApp::update()
{
  // if mCurInstagram is undefined, then don't do anything else since there's nothing else to do
	if( mCurInstagram.isNull() ) {
		newInstagram();
		return;
	}
	
	// if the new texture is loading, but hasn't loaded yet, check again
	if( ! mTextureLoaded ) {
		checkImageLoaded(); // image is loaded, texture is made for triangles
	}
	else {
		// we want to call this only once the image has been loaded
		// if the texture has been loaded and the phase hasn't been called to change yet...
		if( ! mPhaseChangeCalled )
			imageLoaded(); // switch between mirror and still image modes. also initiates the first call to phase change
			if (mDrawingMirrorTex) updateMirrors( &mTriPieces ); // update sample each frame update if in the mirror mode
	}

	// draw debug GUI
	if (isInDebugMode){
		ui::ScopedWindow window( "Settings" );

		if (ui::CollapsingHeader("Debug", nullptr, true, true)) {
			ui::Checkbox("Draw original image in the background", &isDrawingOriginalImage);
			ui::Checkbox("Only draw the nth hexagon", &isDrawingOneHexagon);
			if (isDrawingOneHexagon) {
				ui::SliderInt("n", &nthHexagon, 0, 25); // only first 25 because it's for debugging purpose only
				ui::SliderInt( "triangle index (6 = all)", &tri_index, 0, 6 ); // 6 if drawing all triangles
			}
		}
		if (ui::CollapsingHeader("Animation", nullptr, true, true)){
			ui::Checkbox("Show heart cutout", &isDrawingHeartCutout);
			ui::Checkbox("Disable global rotation", &isDisablingGlobalRotation);
			ui::Checkbox("Randomize the hexagon initialization", &isRandomizingHexInitalization);
			ui::Checkbox("Rotate the hexagon by 30 degrees", &isRotatingHexagon);
			ui::Checkbox("Use the box texture", &isUsingBoxTexture);
			ui::Checkbox("Twinkle with opacity", &isTwinklingWithOpacity);
			ui::Checkbox("Mouse interaction", &isMousing);
			if(isMousing){
				ui::SliderFloat("sample size", &mSampleSize, 50.0, 300.0); //sample size
				ui::SliderFloat("sample point x", &mSamplePt.value().x, 0, getWindowWidth());
				ui::SliderFloat("sample point y", &mSamplePt.value().y, 0, getWindowHeight());

			}
		}
	}
}

void SosoValentinesApp::updateMirrors( vector<TrianglePiece> *vec )
{
	if( ! mMirrorTexture ) return;

	vec2 mSamplePt1( -0.5, -(sin(M_PI/3)/3) );
	vec2 mSamplePt2( mSamplePt1.x + 1, mSamplePt1.y);
	vec2 mSamplePt3( mSamplePt1.x + (cos(M_PI/3)), mSamplePt1.y + (sin(M_PI/3)));

	mat3 mtrx( 1.0f );

	// this part controls the sampling motion
	mtrx = glm::translate( mtrx, mSamplePt.value() );
	mtrx = glm::scale( mtrx, vec2( mSampleSize ) );
	mtrx = glm::rotate( mtrx, float((getElapsedFrames()*4)/2*M_PI) );
	
	mSamplePt1 = vec2( mtrx * vec3( mSamplePt1, 1.0 ) );
	mSamplePt2 = vec2( mtrx * vec3( mSamplePt2, 1.0 ) );
	mSamplePt3 = vec2( mtrx * vec3( mSamplePt3, 1.0 ) );
	
	mSamplePt1 /= mMirrorTexture->getSize();
	mSamplePt2 /= mMirrorTexture->getSize();
	mSamplePt3 /= mMirrorTexture->getSize();
	
	// loop through all the pieces and pass along the current texture and its coordinates
	int outCount = 0;
	int inCount = 0;
	for( int i = 0; i < vec->size(); i++ ) {
		(*vec)[i].update( mMirrorTexture, mSamplePt1, mSamplePt2, mSamplePt3 );
		if( (*vec)[i].isOut() ) outCount++;
		if( (*vec)[i].isIn() ) {
			inCount++;
		}
	}
	//cout << "INCOUNT" << inCount << ", OUTCOUNT " << outCount  << ", mPiecesIn " << mPiecesIn<< endl;
  //cout << "size " << mTriPieces.size() <<endl;
  
	// if all are out, then make a new mirror grid
	if( outCount > 0 && outCount == mTriPieces.size() ) {
		mirrorOut();
	}
	
	// if all the pieces are in
	if( inCount > 0 && inCount == mTriPieces.size() && ! mPiecesIn ) {
		mPiecesIn = true;
		mirrorIn();
	}
}

void SosoValentinesApp::mirrorOut()
{
	cout << "mirror out" << endl;
  continueCycle();
}

void SosoValentinesApp::mirrorIn()
{
	cout << "mirror in" << endl;
	// redefine the bg texture
	mBgTexture = mNewTex;
	mTextRibbon->update( TAG, mCurInstagram.getUser() );

	for ( auto &piece: mTriPieces) {
		piece.setTransitionOut(MIRROR_DUR - 0.5f);
	}
}

void SosoValentinesApp::draw()
{
	gl::clear( Color( 1.0f, 1.0f, 1.0f ) );
	gl::enableAlphaBlending( PREMULT );

  // draw original image
	if( mBgTexture && isDrawingOriginalImage ) {
		gl::draw( mBgTexture, Rectf( mBgTexture->getBounds() ).getCenteredFit( getWindowBounds(), true ) );
	}

	if (mDrawingMirrorTex) {
		cout << "drawing mirrors" << endl;
		drawMirrors( &mTriPieces );
	}

	// heart cutout should always be on top (under the text)
	if (isDrawingHeartCutout)
	{
			gl::draw( mHeartTexture, Rectf( mHeartTexture->getBounds() ).getCenteredFit( getWindowBounds(), true ) );
	}
	mTextRibbon->draw();
}

void SosoValentinesApp::drawMirrors( vector<TrianglePiece> *vec )
{
	gl::ScopedModelMatrix scopedMat;
	gl::translate( getWindowCenter() ); // draws the texture in the middle of the screen

	// texture global rotation
	if (!isDisablingGlobalRotation) {
		gl::rotate( mMirrorRot );
	}
	if ( isDrawingOneHexagon ) {
		if ( tri_index < 6 ){
			//draw the triangle piece at index tri_index of the nth hexagon
			(*vec)[ (nthHexagon * 6 ) + tri_index ].draw();
			// draw the center dot of the hexagon
			gl::pushModelMatrix();
			gl::color(Color(1.0f, 0, 0));
			gl::drawSolidCircle((*vec)[tri_index].mStartPt, 5.0f);
			gl::popModelMatrix();

			// cout << "center " << (*vec)[0].mStartPt << endl;
			} else {
			// draw the full hex for tri_index = 6 * i
			for( int i = (nthHexagon * 6) ; i < (6 * (nthHexagon + 1)); i++ ) {
				(*vec)[i].draw();
			}
		}	// end of isDrawingOneHexagon
	}
	else {
		for( int i = 0; i < vec->size(); i++ ) {
				(*vec)[i].draw();
		}
	}
}

// mouse functions
void SosoValentinesApp::mouseDrag(MouseEvent event)
{
	// Store the current mouse position in the list.
	mousePoints.push_back( event.getPos() );
}

void SosoValentinesApp::mouseDown(MouseEvent event){
	mousePos = event.getPos();
}

bool SosoValentinesApp::inTriangleCheck(ci::vec2 mVertices[3], ci::vec2 mousePos){
	float x_p1 = mVertices[0].x;
	float x_p2 = mVertices[1].x;
	float x_p3 = mVertices[2].x;
	float x_m = mousePos.x;

	float y_p1 = mVertices[0].y;
	float y_p2 = mVertices[1].y;
	float y_p3 = mVertices[2].y;
	float y_m = mousePos.y;

	int a = ( (y_p2-y_p3)*(x_m-x_p3) + (x_p3-x_p2)*(y_m-y_p3) )/( (y_p2-y_p3)*(x_p1-x_p3) + (x_p3-x_p2)*(y_p1-y_p3) );
	int b = ( (y_p3-y_p1)*(x_m-x_p3) + (x_p1-x_p3)*(y_m-y_p3) )/( (y_p2-y_p3)*(x_p1-x_p3) + (x_p3-x_p2)*(y_p1-y_p3) );
	int c = 1-a-b;

	// Apply test
	if( (a<=1 && a>=0) && (b<=1 && b>=0) && (c<=1 && c>=0) )
		return true;
	else
		return false;
	
}

CINDER_APP( SosoValentinesApp, RendererGl( RendererGl::Options().msaa( 16 ) ) )
