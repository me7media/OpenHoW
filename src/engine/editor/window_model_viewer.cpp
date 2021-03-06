/* OpenHoW
 * Copyright (C) 2017-2020 Mark Sowden <markelswo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../engine.h"
#include "../graphics/shaders.h"
#include "../graphics/display.h"
#include "../imgui_layer.h"
#include "../model.h"

#include "window_model_viewer.h"
#include "window_texture_viewer.h"

std::list<std::string> ModelViewer::modelList;

ModelViewer::ModelViewer() : BaseWindow() {
	const char **formatExtensions = supported_model_formats;
	while( *formatExtensions != nullptr ) {
		plScanDirectory( "chars", *formatExtensions, &ModelViewer::AppendModelList, true );
		formatExtensions++;
	}

	modelList.sort();
	modelList.unique();

	drawBuffer = plCreateFrameBuffer( 640, 480, PL_BUFFER_COLOUR | PL_BUFFER_DEPTH );
	if ( drawBuffer == nullptr ) {
		Error( "Failed to create framebuffer for model viewer (%s)!\n", plGetError() );
	}

	textureAttachment = plGetFrameBufferTextureAttachment( drawBuffer );
	if ( textureAttachment == nullptr ) {
		Error( "Failed to create texture attachment for buffer (%s)!\n", plGetError() );
	}

	camera = new Camera( { -2500.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } );
	camera->SetViewport( { 0, 0 }, { 640, 480 } );
}

ModelViewer::~ModelViewer() {
	plDestroyModel( modelPtr );
	plDestroyTexture( textureAttachment );
	plDestroyFrameBuffer( drawBuffer );
}

void ModelViewer::DrawViewport() {
	plBindFrameBuffer( drawBuffer, PL_FRAMEBUFFER_DRAW );

	plSetDepthBufferMode( PL_DEPTHBUFFER_ENABLE );

	plSetClearColour( { 0, 0, 0, 0 } );
	plClearBuffers( PL_BUFFER_COLOUR | PL_BUFFER_DEPTH );

	if ( modelPtr == nullptr ) {
		plBindFrameBuffer( nullptr, PL_FRAMEBUFFER_DRAW );
		return;
	}

	camera->MakeActive();

	ShaderProgram *shaderProgram = Shaders_GetProgram( viewDebugNormals ? "debug_normals" : "generic_textured_lit" );
	shaderProgram->Enable();

	if ( !viewDebugNormals ) {
		plSetNamedShaderUniformVector3( shaderProgram->GetInternalProgram(), "sun_position",
			PLVector3( 0.5f, 0.2f, 0.6f ) );
		plSetNamedShaderUniformVector4( shaderProgram->GetInternalProgram(), "sun_colour",
			PLColour( 255, 255, 255, 255 ).ToVec4() );
		plSetNamedShaderUniformVector4( shaderProgram->GetInternalProgram(), "ambient_colour",
			PLColour( 128, 128, 128, 255 ).ToVec4() );
	}

	PLVector3 angles(
		plDegreesToRadians( modelRotation.x ),
		plDegreesToRadians( modelRotation.y ),
		plDegreesToRadians( modelRotation.z ) );

	PLMatrix4 *matrixPtr = &modelPtr->model_matrix;
	matrixPtr->Identity();
	matrixPtr->Rotate( angles.z, { 1, 0, 0 } );
	matrixPtr->Rotate( angles.y, { 0, 1, 0 } );
	matrixPtr->Rotate( angles.x, { 0, 0, 1 } );

	plDrawModel( modelPtr );

	plBindFrameBuffer( nullptr, PL_FRAMEBUFFER_DRAW );
}

void ModelViewer::Display() {
	// Draw the model view if there's a valid model
	DrawViewport();

	if ( viewRotate ) {
		modelRotation.y += 0.01f * g_state.last_draw_ms;
	}

	ImGui::SetNextWindowSize( ImVec2( 640, 480 ), ImGuiCond_Once );
	ImGui::Begin( dname( "Model Viewer" ), &status_,
				  ImGuiWindowFlags_MenuBar |
					  ImGuiWindowFlags_AlwaysAutoResize |
					  ImGuiWindowFlags_HorizontalScrollbar |
					  ImGuiWindowFlags_NoSavedSettings );

	if ( ImGui::BeginMenuBar() ) {
		if ( ImGui::BeginMenu( "Models" ) ) {
			for ( const auto &i : modelList ) {
				bool selected = false;
				if ( modelPtr != nullptr ) {
					selected = ( i == modelPtr->path );
				}

				if ( ImGui::Selectable( i.c_str(), selected ) ) {
					plDestroyModel( modelPtr );
					modelPtr = nullptr;

					modelPtr = plLoadModel( i.c_str() );
					if ( modelPtr == nullptr ) {
						LogWarn( "Failed to load \"%s\" (%s)!\n", i.c_str(), plGetError() );
					}
				}
			}
			ImGui::EndMenu();
		}

		// Urgh, yeah this will eventually need to change
		if ( ImGui::BeginMenu( "Animations" ) ) {
			for ( unsigned int i = 0; static_cast<AnimationIndex>(i) < AnimationIndex::MAX_ANIMATIONS; ++i ) {
				const char *animationName = Model_GetAnimationDescription( i );
				if ( animationName == nullptr ) {
					continue;
				}

				bool selected = false;
				if ( ImGui::Selectable( animationName, selected ) ) {
					// todo
				}
			}
			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "View" ) ) {
			if ( ImGui::MenuItem( "Fullscreen", nullptr, viewFullscreen ) ) {
				viewFullscreen = !viewFullscreen;

				//plDestroyTexture( textureAttachment );
				//plDestroyFrameBuffer( drawBuffer );

				if ( viewFullscreen ) {

				}
			}
			ImGui::Separator();
			if ( ImGui::MenuItem( "Rotate Model", nullptr, viewRotate ) ) {
				viewRotate = !viewRotate;
			}
			if ( ImGui::MenuItem( "Debug Normals", nullptr, viewDebugNormals ) ) {
				viewDebugNormals = !viewDebugNormals;
			}

			if ( modelPtr != nullptr ) {
				ImGui::Separator();

				if ( ImGui::BeginMenu( "Textures" ) ) {
					for ( unsigned int i = 0; i < modelPtr->levels[0].num_meshes; ++i ) {
						PLMesh *mesh = modelPtr->levels[0].meshes[i];
						if ( ImGui::ImageButton( reinterpret_cast<ImTextureID>( mesh->texture->internal.id ),
							ImVec2( 128, 128 ) ) ) {
							ImGuiImpl_RegisterWindow( new TextureViewer( mesh->texture ) );
						}
					}

					ImGui::EndMenu();
				}
			}

			ImGui::Separator();

			ImGui::Text( "Camera Properties" );
			float fov = camera->GetFieldOfView();
			if ( ImGui::SliderFloat( "Fov", &fov, 0.0f, 180.0f, "%.f" ) ) {
				camera->SetFieldOfView( fov );
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

#if 0
	if ( ImGui::ListBoxHeader( "Models", modelList.size() ) ) {

		ImGui::ListBoxFooter();
	}

	ImGui::SameLine();
#endif

	ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonActive, ImVec4( 0, 0, 0, 0 ) );
	ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImVec4( 0, 0, 0, 0 ) );

	ImGui::ImageButton( reinterpret_cast<ImTextureID>(textureAttachment->internal.id), ImVec2( 640, 480 ) );
	if ( ImGui::IsItemHovered() ) {
		float newXPos = ImGui::GetMousePos().x - oldMousePos[0];
		float newYPos = ImGui::GetMousePos().y - oldMousePos[1];
		const static float posMod = 200.0f;
		if ( ImGui::IsMouseDown( ImGuiMouseButton_Left ) ) { // Rotation
			modelRotation.x += newYPos / posMod * g_state.last_draw_ms;
			modelRotation.y += newXPos / posMod * g_state.last_draw_ms;
		} else if ( ImGui::IsMouseDown( ImGuiMouseButton_Middle ) ) { // Panning
			PLVector3 position = camera->GetPosition();
			position.z += newXPos / posMod * g_state.last_draw_ms;
			position.y += newYPos / posMod * g_state.last_draw_ms;
			camera->SetPosition( position );
		} else {
			oldMousePos[0] = ImGui::GetMousePos().x;
			oldMousePos[1] = ImGui::GetMousePos().y;
		}

		// Handle mouse wheel movements
		PLVector3 position = camera->GetPosition();
		position.x += ImGui::GetIO().MouseWheel * 10;
		camera->SetPosition( position );
	}

	ImGui::PopStyleColor( 3 );

	ImGui::End();
}

void ModelViewer::AppendModelList( const char *path ) {
	modelList.push_back( path );
}
