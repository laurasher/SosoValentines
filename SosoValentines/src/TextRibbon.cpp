//
//	TextRibbon.cpp
//	Instascope
//
//	Created by Greg Kepler on 6/26/12.
//	Copyright (c) 2012 The Barbarian Group. All rights reserved.
//

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Batch.h"
#include "cinder/Text.h"
#include "cinder/Timeline.h"
#include "string"

#include "TextRibbon.h"
#include "Resources.h"

static const int WINDOW_WIDTH = 1080;

using namespace std;
using namespace ci;
using namespace ci::app;

TextRibbon::TextRibbon()
: mCol(Color::black()), mTextCol(Color::black())
{
	mUserFont = Font( loadResource( RES_KREON_BOLD ), 35 );
	//		mUserFont = Font( loadResource( BOLD ), 30 );
	mTagFont = Font( loadResource( LOVELICA ), 40 );
	//	mTagFont = Font( loadResource( RES_OPEN_SANS ), 20 );

	auto text_bg_img = loadImage( loadAsset("text_background.png") );
	text_background_tex = gl::Texture2d::create(text_bg_img);
}

void TextRibbon::update( string user, string mSearchTag )
{
	//clear previous user text
	mTrimTag.clear();
	mTag = "#" + mSearchTag;
	// trim the fat
	int i = 0,trimCount=0;
	while(i<user.length()){
		if (user[i]=='#'||user[i]=='@'){ //single quotes used for chars, double for strings
			//jump to space
			while(user[i]!=' '){
				i++;
			}
		} else{
			mTrimTag += user[i];
			i++; trimCount++;
		}
	}

	console()<<mTrimTag<<endl;
	//	mUser = user;
	mUser = mTrimTag;


	mCurPos.value() = vec2(-60, 0);
	mCurAlpha = 0.0f;

	makeText();

	ribbonIn( 4.0 );
}

void TextRibbon::ribbonIn( float delay )
{
	// animate ribbon in
	app::timeline().apply( &mCurAlpha, 1.0f, 0.4f, EaseOutQuint()).delay(delay);
	app::timeline().apply( &mCurPos, vec2( 0, 0 ), 0.4f, EaseOutQuint()).delay(delay);
}

void TextRibbon::ribbonOut( float delay )
{
	// animate ribbon out
	app::timeline().apply( &mCurAlpha, 0.0f, 0.4f, EaseInQuint()).delay(delay);
	app::timeline().apply( &mCurPos, vec2(-60, 0), 0.4f, EaseInQuint()).delay(delay);
}

void TextRibbon::makeText()
{
	// reset the textures
	mTagTex.reset();
	mUserTex.reset();

	// Create the texture for the text
	if( ! mTag.empty() ) {

		mTagBox = TextBox().alignment(TextBox::CENTER).font(mTagFont).size(ivec2( WINDOW_WIDTH , TextBox::GROW)).text(mTag);
		mTagBox.setColor(ColorA(mTextCol.r, mTextCol.g, mTextCol.b, 1));
		mTagBox.setBackgroundColor(ColorA(0, 0, 0, 0));
		mTagTex = gl::Texture::create( mTagBox.render() );
	}

	mUserBox = TextBox().alignment( TextBox::CENTER ).font( mUserFont ).size( ivec2( WINDOW_WIDTH-20, TextBox::GROW ) ).text( mUser );

	mUserBox.setColor(ColorA(mTextCol.r, mTextCol.g, mTextCol.b, 1));
	mUserBox.setBackgroundColor( ColorA( 0, 0, 0, 0) );
	mUserTex = gl::Texture::create( mUserBox.render() );

//	float ribbonWidth = mTagBox.measure().x + mUserBox.measure().x + 30;
//	mRibbonSize = vec2(ribbonWidth, 100);
//	mTextPos = vec2(0, getWindowHeight() - mRibbonSize.y - 500);
}


void TextRibbon::draw()
{
	gl::ScopedModelMatrix scopedMat;
	gl::ScopedColor scopedColor;
	gl::translate(mTextPos);
  	
	float spacing = 0;
	gl::color( 1, 1, 1, mCurAlpha );

	// background image
	auto rect = Rectf( text_background_tex->getBounds() ).getCenteredFit( getWindowBounds(), false );
	//rect.offset(vec2(0.0f, -69.282f * 1.5));
	rect.offset(vec2(0.0f, -60.0f * 0.33));

	gl::draw( text_background_tex, rect );
	//gl::draw( text_background_tex, Rectf( text_background_tex->getBounds() ).getCenteredFit( getWindowBounds(), false ) );
	//gl::draw( text_background_tex, Rectf( text_background_tex->getBounds() ).getCenteredFit( getWindowBounds(), true ) );
// testing

	// Now draw the text textures:
	// check it the texture exists and if mTagBox has a height (meaning that there's something in that texture)
	if( mUserTex ){
		auto textRect = Rectf( mUserTex->getBounds() ).getCenteredFit( getWindowBounds(), false );
		//rect.offset(vec2(0.0f, -69.282f * 1.5));
		textRect.offset(vec2(0.0f, -60.0f * 0.33 + (-60*1)));

		gl::draw( mUserTex, textRect );

		//gl::draw( mUserTex, vec2( (0) , ((mUserBox.measure().y)/2)-150) );
	}

	if( mTagTex && mTagBox.measure().y > 0 ) {
		spacing = 5;
		//gl::draw( mTagTex, vec2( (0) , ((mTagBox.measure().y)/2)+ (mUserBox.measure().y)-30  ));
		auto textRect = Rectf( mTagTex->getBounds() ).getCenteredFit( getWindowBounds(), false );
		//rect.offset(vec2(0.0f, -69.282f * 1.5));
		//textRect.offset(vec2(0.0f, -60.0f * 0.33));
		textRect.offset(vec2(0.0f, 60.0f ));
		gl::draw( mTagTex, textRect );
	}
}
