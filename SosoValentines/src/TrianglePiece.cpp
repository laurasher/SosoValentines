//
//	TrianglePiece.cpp
//	Instascope
//
//	Created by Greg Kepler on 5/30/12.
//	Copyright (c) 2012 The Barbarian Group. All rights reserved.
//

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Shader.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "TrianglePiece.h"

using namespace ci;
using namespace ci::app;
using namespace std;

TrianglePiece::TrianglePiece( vec2 _startPt, vec2 _pt1, vec2 _pt2, vec2 _pt3, float _rotation, vec2 _scale , Anim<float> _alpha)
	: mTransOut(false)
{
	mStartPt = _startPt;
	mVertices[0] = _pt1;
	mVertices[1] = _pt2;
	mVertices[2] = _pt3;
	mRotation = _rotation;
	mScale = _scale;
	mAlpha = _alpha;
	mReadyToDraw = false;
	mVisible = false;
	mTransOut = false;

}

void TrianglePiece::reset( float _delay, gl::TextureRef tempTex )
{
	mTempTex = tempTex;
	setTransition(_delay);
}

void TrianglePiece::setTransition( float _delay )
{
	if( mVisible ){
		// transition out
		mTransOut = true;
		mDrawTex = mTempTex;
		app::timeline().apply( &mAlpha, 0.0f, randFloat(0.2f, 0.5f), EaseInQuint()).delay(_delay);	// make sure it's long enough

		//cout << "mAlpha " << mAlpha << endl;
		//setTransitionOut(5.0f - 0.5f);
		// hardcode for now


		//cout << "transitioning out " << timeline().getCurrentTime()<<endl;
		//cout << "in setTansition:: mTransout " << mTransOut << endl;
	}
	else {
		// transition in

		// can add box texture thing here
		float alpha = 0.0f;
		if (mRotation == M_PI / 3 * 0 ||  mRotation == M_PI / 3 * 1)  {alpha = 0.7f;}
		else if (mRotation == M_PI / 3 * 2 ||  mRotation == M_PI / 3 * 5)  {alpha = 0.85f;}
		else {alpha = 1.0f;}
		// the alpha needs to be dark enough. change to saturation?

		float firstTwinkle = randFloat(0.5f, 1.0f);
		// for twinkling entrance
		app::timeline().apply( &mAlpha, alpha, firstTwinkle , EaseInQuint()).delay(0.5f + _delay);
		// make sure it goes to opacity of 1.0
		app::timeline().apply( &mAlpha, 1.0f, randFloat(1.0f, 2.0f), EaseInQuint()).delay(0.5f + _delay + firstTwinkle).
		startFn( [this] { setVisible( true ); } );
	}
}

void TrianglePiece::setTransitionOut( float _delay )
{
	setTransition( _delay );
	//hasTransitionedIn = false; // default back to false to get ready for transitionMirrorIn
}

void TrianglePiece::setVisible( bool vis )
{
	mVisible = vis;
}

void TrianglePiece::update( gl::TextureRef tex, vec2 pt1, vec2 pt2, vec2 pt3 )
{
	if( ! mTransOut ) {
		mTexVertices[0] = pt1;
		mTexVertices[1] = pt2;
		mTexVertices[2] = pt3;
		mDrawTex = tex;
	}
	mReadyToDraw = true;
}

void TrianglePiece::draw()
{
	if( ! mReadyToDraw ) return;
	if( mAlpha == 0.0f ) return;

	gl::ScopedModelMatrix scopedMat;
	gl::ScopedColor scopedCol;
	gl::translate( mStartPt );	// move to the start point
	gl::scale( mScale );		// scale the triangle
	gl::rotate( mRotation );	// rotate on the Z axis

	// set individual saturation, color here
	float twinkle = 0.0;
	twinkle = randFloat(-0.05, 0.05);

	//this one gets RBGA from the original image(that's why it's not white)
	//gl::color( 1.0, 1.0, 1.0, mAlpha);
	gl::color( 1.0, 1.0, 1.0, mAlpha);
	//gl::color( Color(CM_HSV, 1.0, 1.0, 1.0)); // substitute V for alpha
	//gl::color( 1.0, 1.0, 1.0, mAlpha + twinkle); //twinkle with random opacity

	// draw the texture to the triangle
	gl::ScopedGlslProg glslScp( gl::getStockShader( gl::ShaderDef().color().texture() ) );
	gl::ScopedTextureBind texScp( mDrawTex );
	gl::drawSolidTriangle( mVertices, mTexVertices );
}

bool TrianglePiece::isOut() const
{
	// had to round here because every once in a while, it's something like .00001
	bool out = mTransOut && mAlpha <= 0.1;

	//cout << "in ISOUT :: mTransout " << mTransOut << " hasTransitionedIn " << hasTransitionedIn << endl;

	//bool out = mTransOut && !(hasTransitionedIn); // don't care about mAlpha
	// bool out = mTransOut;
	return out;
	}

bool TrianglePiece::isIn() const
{
	//cout << "mAlpha " << mAlpha << endl;
	//cout << "triangle piece is in. malpha is " << mAlpha << " and mTransOut is " << mTransOut<<endl;
	return ( ! mTransOut && mAlpha >= 0.9 ); // 0.4 because the lightest opacity of the box texture
	// cout << "mtransout " << mTransOut  << "& hasTransitionedin " << hasTransitionedIn << endl;
	//return ( ! mTransOut && hasTransitionedIn );
	//return ( ! mTransOut );
}
