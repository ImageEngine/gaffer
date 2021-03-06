//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2019, John Haddon. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "GafferUI/AnnotationsGadget.h"

#include "GafferUI/GraphGadget.h"
#include "GafferUI/ImageGadget.h"
#include "GafferUI/NodeGadget.h"
#include "GafferUI/Style.h"

#include "Gaffer/Metadata.h"
#include "Gaffer/MetadataAlgo.h"

#include "boost/algorithm/string/predicate.hpp"
#include "boost/bind.hpp"
#include "boost/bind/placeholders.hpp"

using namespace GafferUI;
using namespace Gaffer;
using namespace IECore;
using namespace Imath;
using namespace std;

//////////////////////////////////////////////////////////////////////////
// Internal utilities
//////////////////////////////////////////////////////////////////////////

namespace
{

Box2f nodeFrame( const NodeGadget *nodeGadget )
{
	const Box3f b = nodeGadget->transformedBound( nullptr );
	return Box2f(
		V2f( b.min.x, b.min.y ),
		V2f( b.max.x, b.max.y )
	);
}


IECoreGL::Texture *bookmarkTexture()
{
	static IECoreGL::TexturePtr bookmarkTexture;

	if( !bookmarkTexture )
	{
		bookmarkTexture = ImageGadget::textureLoader()->load( "bookmarkStar2.png" );

		IECoreGL::Texture::ScopedBinding binding( *bookmarkTexture );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
	}
	return bookmarkTexture.get();
}

IECoreGL::Texture *numericBookmarkTexture()
{
	static IECoreGL::TexturePtr numericBookmarkTexture;

	if( !numericBookmarkTexture )
	{
		numericBookmarkTexture = ImageGadget::textureLoader()->load( "bookmarkStar.png" );

		IECoreGL::Texture::ScopedBinding binding( *numericBookmarkTexture );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
	}
	return numericBookmarkTexture.get();
}

float g_offset = 0.5;
float g_borderWidth = 0.5;
float g_spacing = 0.25;

} // namespace

//////////////////////////////////////////////////////////////////////////
// AnnotationsGadget
//////////////////////////////////////////////////////////////////////////

GAFFER_GRAPHCOMPONENT_DEFINE_TYPE( AnnotationsGadget );

AnnotationsGadget::AnnotationsGadget()
	:	Gadget( "AnnotationsGadget" )
{
	Metadata::nodeValueChangedSignal().connect(
		boost::bind( &AnnotationsGadget::nodeMetadataChanged, this, ::_1, ::_2, ::_3 )
	);
}

AnnotationsGadget::~AnnotationsGadget()
{
}

bool AnnotationsGadget::acceptsParent( const GraphComponent *potentialParent ) const
{
	return runTimeCast<const GraphGadget>( potentialParent );
}

void AnnotationsGadget::parentChanging( Gaffer::GraphComponent *newParent )
{
	m_annotations.clear();
	m_graphGadgetChildAddedConnection.disconnect();
	m_graphGadgetChildRemovedConnection.disconnect();
	if( newParent )
	{
		m_graphGadgetChildAddedConnection = newParent->childAddedSignal().connect(
			boost::bind( &AnnotationsGadget::graphGadgetChildAdded, this, ::_2 )
		);
		m_graphGadgetChildRemovedConnection = newParent->childRemovedSignal().connect(
			boost::bind( &AnnotationsGadget::graphGadgetChildRemoved, this, ::_2 )
		);
	}
}

void AnnotationsGadget::doRenderLayer( Layer layer, const Style *style ) const
{
	if( layer != GraphLayer::Overlay )
	{
		return;
	}

	vector<InternedString> registeredValues;
	for( auto &ga : m_annotations )
	{
		const Node *node = ga.first->node();
		Annotations &annotations = ga.second;

		if( annotations.dirty )
		{
			annotations.renderable = false;

			annotations.bookmarked = Gaffer::MetadataAlgo::getBookmarked( node );
			annotations.renderable |= annotations.bookmarked;

			if( int bookmark = MetadataAlgo::numericBookmark( node ) )
			{
				annotations.numericBookmark = std::to_string( bookmark );
				annotations.renderable = true;
			}
			else
			{
				annotations.numericBookmark = InternedString();
			}

			annotations.standardAnnotations.clear();
			registeredValues.clear();
			Metadata::registeredValues( node, registeredValues );
			for( const auto &key : registeredValues )
			{
				if( boost::starts_with( key.string(), "annotation:" ) && boost::ends_with( key.string(), ":text" ) )
				{
					if( auto text = Metadata::value<StringData>( node, key ) )
					{
						const string prefix = key.string().substr( 0, key.string().size() - 4 );
						annotations.standardAnnotations.push_back(
							{ text, Metadata::value<Color3fData>( node, prefix + "color" ) }
						);
						annotations.renderable = true;
					}
				}
			}

			annotations.dirty = false;
		}

		if( !annotations.renderable )
		{
			continue;
		}

		const Box2f b = nodeFrame( ga.first );
		if( annotations.bookmarked )
		{
			style->renderImage( Box2f( V2f( b.min.x - 1.0, b.max.y - 1.0 ), V2f( b.min.x + 1.0, b.max.y + 1.0 ) ), bookmarkTexture() );
		}

		if( annotations.numericBookmark.string().size() )
		{
			if( !annotations.bookmarked )
			{
				style->renderImage( Box2f( V2f( b.min.x - 1.0, b.max.y - 1.0 ), V2f( b.min.x + 1.0, b.max.y + 1.0 ) ), numericBookmarkTexture() );
			}

			const Box3f textBounds = style->textBound( Style::LabelText, annotations.numericBookmark.string() );

			const Imath::Color4f textColor( 1.0f );
			glPushMatrix();
				IECoreGL::glTranslate( V2f( b.min.x + 1.0 - textBounds.size().x * 0.5, b.max.y - textBounds.size().y * 0.5 - 0.7 ) );
				style->renderText( Style::BodyText, annotations.numericBookmark.string(), Style::NormalState, &textColor );
			glPopMatrix();
		}

		if( annotations.standardAnnotations.size() )
		{
			glPushMatrix();
			IECoreGL::glTranslate( V2f( b.max.x + g_offset + g_borderWidth, b.max.y - g_borderWidth ) );

			const Color4f midGrey( 0.65, 0.65, 0.65, 1.0 );
			const Color3f darkGrey( 0.05 );
			float previousHeight = 0;
			for( const auto &a : annotations.standardAnnotations )
			{
				Box3f textBounds = style->textBound( Style::BodyText, a.text->readable() );

				float yOffset;
				if( &a == &annotations.standardAnnotations.front() )
				{
					yOffset = -style->characterBound( Style::BodyText ).max.y;
				}
				else
				{
					yOffset = -previousHeight -g_spacing;
				}

				IECoreGL::glTranslate( V2f( 0, yOffset ) );
				/// \todo We're using `renderNodeFrame()` because it's the only way we can specify a colour,
				/// but really we want `renderFrame()` to provide that option. Or we could consider having
				/// explicit annotation rendering methods in the Style class.
				style->renderNodeFrame(
					Box2f( V2f( 0, textBounds.min.y ), V2f( textBounds.max.x, textBounds.max.y ) ),
					g_borderWidth, Style::NormalState,
					a.color ? &(a.color->readable()) : &darkGrey
				);
				style->renderText( Style::BodyText, a.text->readable(), Style::NormalState, &midGrey );
				previousHeight = textBounds.size().y + g_borderWidth * 2;
			}
			glPopMatrix();
		}
	}
}

GraphGadget *AnnotationsGadget::graphGadget()
{
	return parent<GraphGadget>();
}

const GraphGadget *AnnotationsGadget::graphGadget() const
{
	return parent<GraphGadget>();
}

void AnnotationsGadget::graphGadgetChildAdded( GraphComponent *child )
{
	if( NodeGadget *nodeGadget = runTimeCast<NodeGadget>( child ) )
	{
		m_annotations[nodeGadget] = Annotations();
	}
}

void AnnotationsGadget::graphGadgetChildRemoved( const GraphComponent *child )
{
	if( const NodeGadget *nodeGadget = runTimeCast<const NodeGadget>( child ) )
	{
		m_annotations.erase( nodeGadget );
	}
}

void AnnotationsGadget::nodeMetadataChanged( IECore::TypeId nodeTypeId, IECore::InternedString key, Gaffer::Node *node )
{
	if( !node )
	{
		// We only expect annotations to be registered
		// as per-instance metadate.
		return;
	}

	if(
		!MetadataAlgo::bookmarkedAffectedByChange( key ) &&
		!MetadataAlgo::numericBookmarkAffectedByChange( key ) &&
		!boost::starts_with( key.c_str(), "annotation:" )
	)
	{
		return;
	}

	if( auto gadget = graphGadget()->nodeGadget( node ) )
	{
		auto it = m_annotations.find( gadget );
		assert( it != m_annotations.end() );
		it->second.dirty = true;
		dirty( DirtyType::Render );
	}
}
