//////////////////////////////////////////////////////////////////////////
//  
//  Copyright (c) 2012, John Haddon. All rights reserved.
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

#ifndef GAFFERSCENE_RENDERABLESOURCE_H
#define GAFFERSCENE_RENDERABLESOURCE_H

#include "Gaffer/TypedObjectPlug.h"
#include "Gaffer/TransformPlug.h"

#include "GafferScene/SceneNode.h"

namespace GafferScene
{

/// \todo Support turning IECore::Groups into a proper scene hierarchy.
template<typename BaseType>
class RenderableSource : public BaseType
{

	public :

		IECORE_RUNTIMETYPED_DECLARETEMPLATE( RenderableSource<BaseType>, BaseType );
		IE_CORE_DECLARERUNTIMETYPEDDESCRIPTION( RenderableSource<BaseType> );

		virtual ~RenderableSource();
		
		Gaffer::StringPlug *namePlug();
		const Gaffer::StringPlug *namePlug() const;

		Gaffer::TransformPlug *transformPlug();
		const Gaffer::TransformPlug *transformPlug() const;
		
		virtual void affects( const Gaffer::ValuePlug *input, Gaffer::Node::AffectedPlugsContainer &outputs ) const;
		
	protected :

		RenderableSource( const std::string &name, const std::string &namePlugDefaultValue );

		Gaffer::ObjectPlug *renderablePlug();
		const Gaffer::ObjectPlug *renderablePlug() const;

		virtual IECore::VisibleRenderablePtr computeRenderable( const Gaffer::Context *context ) const = 0;		
		
		virtual void compute( Gaffer::ValuePlug *output, const Gaffer::Context *context ) const;
		virtual Imath::Box3f computeBound( const SceneNode::ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const;
		virtual Imath::M44f computeTransform( const SceneNode::ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const;
		virtual IECore::PrimitivePtr computeGeometry( const SceneNode::ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const;
		virtual IECore::StringVectorDataPtr computeChildNames( const SceneNode::ScenePath &path, const Gaffer::Context *context, const ScenePlug *parent ) const;

	private :
	
		Gaffer::ObjectPlug *inputRenderablePlug();
		const Gaffer::ObjectPlug *inputRenderablePlug() const;
		
};

typedef RenderableSource<SceneNode> RenderableSceneNode;
IE_CORE_DECLAREPTR( RenderableSceneNode );

} // namespace GafferScene

#endif // GAFFERSCENE_RENDERABLESOURCE_H
