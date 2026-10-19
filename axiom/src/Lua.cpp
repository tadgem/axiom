#include "Script/Lua.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "Assets/Model.hpp"
#include "Assets/TextureAsset.hpp"
#include "Axiom.hpp"
#include "Core/Profile.hpp"
#include "Im3D/im3d_vku.h"
#include "ThirdParty/FunnelSansTTF.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "mimalloc.h"

namespace
{
using namespace axm;

struct LuaPipeline
{
	vku::VkPipelineData m_Data;
};

struct LuaTexture
{
	Texture* m_Texture = nullptr;
};

struct LuaModel
{
	ModelAsset* m_Asset = nullptr;
};

struct UniformMember
{
	std::string                 m_Name;
	u32                         m_Offset;
	u32                         m_Size;
	vku::ShaderBufferMemberType m_Type;
};

struct LuaMaterial
{
	vku::Material                         m_Material;
	HashMap<u64, DynArray<UniformMember>> m_Uniforms;
};

struct LuaUniformBlock
{
	LuaMaterial* m_Material = nullptr;
	u32          m_Set      = 0;
	u32          m_Binding  = 0;
};

struct ScriptContext
{
	AxiomEngine*           m_Engine        = nullptr;
	vku::VkState*          m_VK            = nullptr;
	VkCommandBuffer        m_Cmd           = VK_NULL_HANDLE;
	u32                    m_Frame         = 0;
	VkPipelineLayout       m_Layout        = VK_NULL_HANDLE;
	Viewport               m_Viewport      = {};
	vku::VkuIm3dState*     m_Im3dState     = nullptr;
	vku::VkuIm3dViewState* m_Im3dViewState = nullptr;
	bool                   m_Im3dActive    = false;
	glm::mat4              m_Im3dViewProj  = glm::mat4(1.0f);
	u32                    m_Im3dWidth     = 0;
	u32                    m_Im3dHeight    = 0;
};

ScriptContext g_Context;

u64           UniformKey(u32 set, u32 binding)
{
	return (CAST(set, u64) << 32) | CAST(binding, u64);
}

vku::Material::ShaderBufferBindingData* FindBuffer(vku::Material& material, u32 set, u32 binding)
{
	for (auto& [key, data] : material.m_ShaderBuffers)
	{
		if (key.m_SetBinding.m_Set == set && key.m_SetBinding.m_Binding == binding)
		{
			return &data;
		}
	}
	return nullptr;
}

void WriteMember(u8* dst, const UniformMember& member, sol::object value)
{
	switch (member.m_Type)
	{
		case vku::ShaderBufferMemberType::_float:
		{
			const f32 v = value.as<f32>();
			std::memcpy(dst, &v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_int:
		{
			const i32 v = value.as<i32>();
			std::memcpy(dst, &v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_uint:
		{
			const u32 v = value.as<u32>();
			std::memcpy(dst, &v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_vec2:
		{
			f32 v[2];
			if (value.is<aml::Float2>())
			{
				const aml::Float2 f = value.as<aml::Float2>();
				v[0]                = f.x;
				v[1]                = f.y;
			}
			else if (value.is<sol::table>())
			{
				const sol::table t = value.as<sol::table>();
				v[0]               = t.get_or(1, f32(0));
				v[1]               = t.get_or(2, f32(0));
			}
			else
			{
				return;
			}
			std::memcpy(dst, v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_vec3:
		{
			f32 v[3];
			if (value.is<aml::Vec3>())
			{
				const aml::Vec3 f = value.as<aml::Vec3>();
				v[0]              = f.GetX();
				v[1]              = f.GetY();
				v[2]              = f.GetZ();
			}
			else if (value.is<sol::table>())
			{
				const sol::table t = value.as<sol::table>();
				v[0]               = t.get_or(1, f32(0));
				v[1]               = t.get_or(2, f32(0));
				v[2]               = t.get_or(3, f32(0));
			}
			else
			{
				return;
			}
			std::memcpy(dst, v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_vec4:
		{
			if (!value.is<sol::table>())
			{
				return;
			}
			const sol::table t = value.as<sol::table>();
			f32 v[4]           = {t.get_or(1, f32(0)), t.get_or(2, f32(0)), t.get_or(3, f32(0)), t.get_or(4, f32(0))};
			std::memcpy(dst, v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_mat3:
		{
			if (!value.is<sol::table>())
			{
				return;
			}
			const sol::table t = value.as<sol::table>();
			f32              v[9];
			for (u32 i = 0; i < 9u; ++i)
			{
				v[i] = t.get_or(CAST(i + 1, int), f32(0));
			}
			std::memcpy(dst, v, sizeof(v));
			break;
		}
		case vku::ShaderBufferMemberType::_mat4:
		{
			if (!value.is<sol::table>())
			{
				return;
			}
			const sol::table t = value.as<sol::table>();
			f32              v[16];
			for (u32 i = 0; i < 16u; ++i)
			{
				v[i] = t.get_or(CAST(i + 1, int), f32(0));
			}
			std::memcpy(dst, v, sizeof(v));
			break;
		}
		default:
			break;
	}
}

void WriteUniform(LuaUniformBlock& block, const std::string& name, sol::object value)
{
	if (block.m_Material == nullptr || g_Context.m_VK == nullptr)
	{
		return;
	}

	const auto membersIt = block.m_Material->m_Uniforms.find(UniformKey(block.m_Set, block.m_Binding));
	if (membersIt == block.m_Material->m_Uniforms.end())
	{
		return;
	}

	const UniformMember* found = nullptr;
	for (const auto& member : membersIt->second)
	{
		if (member.m_Name == name.c_str())
		{
			found = &member;
			break;
		}
	}
	if (found == nullptr)
	{
		return;
	}

	vku::Material::ShaderBufferBindingData* buffer =
	        FindBuffer(block.m_Material->m_Material, block.m_Set, block.m_Binding);
	if (buffer == nullptr)
	{
		return;
	}

	const u32 frame  = g_Context.m_VK->m_CurrentFrameIndex;
	auto*     mapped = static_cast<vku::MappedBuffer*>(buffer->m_Buffer.m_UniformBuffers[frame].get());
	if (mapped == nullptr || mapped->m_MappedAddr == nullptr)
	{
		return;
	}

	WriteMember(static_cast<u8*>(mapped->m_MappedAddr) + found->m_Offset, *found, value);
}

LuaMaterial CreateMaterial(vku::ShaderProgram& program)
{
	LuaMaterial material;
	material.m_Material = vku::Material::Create(*g_Context.m_VK, program);

	for (auto& stage : program.m_Stages)
	{
		for (auto& layout : stage.m_LayoutDatas)
		{
			for (auto& binding : layout.m_BindingDatas)
			{
				if (binding.m_BufferType != vku::ShaderBindingType::UniformBuffer &&
				    binding.m_BufferType != vku::ShaderBindingType::ShaderStorageBuffer)
				{
					continue;
				}
				auto& members = material.m_Uniforms[UniformKey(layout.m_SetNumber, binding.m_BindingIndex)];
				for (auto& member : binding.m_Members)
				{
					members.push_back(UniformMember{
					        std::string(member.m_Name.c_str()), member.m_Offset, member.m_Size, member.m_Type});
				}
			}
		}
	}

	return material;
}

Texture* ResolveDiffuse(ModelAsset* asset, size_t index)
{
	if (asset == nullptr || index < 1 || index > asset->m_Data.m_Meshes.size())
	{
		return nullptr;
	}

	const u32   materialIndex = asset->m_Data.m_Meshes[index - 1].m_MaterialIndex;
	const auto& materials     = asset->m_Data.m_Materials;
	if (materialIndex >= materials.size())
	{
		return nullptr;
	}

	for (const auto& map : materials[materialIndex].m_TextureMaps)
	{
		if (map.m_MapType != TextureMapType::Diffuse)
		{
			continue;
		}
		Asset* textureAsset = g_Context.m_Engine->m_AssetManager.GetAsset(map.m_Handle);
		if (auto* concrete = dynamic_cast<TextureAsset*>(textureAsset))
		{
			return &concrete->m_Data;
		}
		return nullptr;
	}

	return nullptr;
}

LuaTexture LoadTextureSync(const char* path)
{
	AssetHandle handle = g_Context.m_Engine->m_AssetManager.LoadAsset(Filesystem::path(path), AssetType::Texture);
	g_Context.m_Engine->m_AssetManager.WaitAllAssets();
	Asset* asset = g_Context.m_Engine->m_AssetManager.GetAsset(handle);
	if (auto* concrete = dynamic_cast<TextureAsset*>(asset))
	{
		return LuaTexture{&concrete->m_Data};
	}
	return LuaTexture{nullptr};
}

Im3d::Vec3 ToIm3d(const aml::Vec3& v)
{
	return Im3d::Vec3(v.GetX(), v.GetY(), v.GetZ());
}

glm::mat4 TableToMat4(sol::table table)
{
	f32 values[16] = {};
	for (u32 i = 0; i < 16u; ++i)
	{
		values[i] = table.get_or(CAST(i + 1, int), f32(0));
	}
	return glm::make_mat4(values);
}

std::unordered_map<std::string, std::array<char, 256>>& InputBuffers()
{
	static std::unordered_map<std::string, std::array<char, 256>> buffers;
	return buffers;
}
}  // namespace

void* axm::lua::LuaCustomAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
	(void) ud;
	(void) osize;

	if (nsize == 0)
	{
		if (ptr != nullptr)
		{
			mi_free(ptr);
		}
		return nullptr;
	}

	return mi_realloc(ptr, nsize);
}

sol::state axm::lua::CreateLuaState(AxiomEngine& engine)
{
	auto state = sol::state(sol::default_at_panic, axm::lua::LuaCustomAllocator, nullptr);
	state.open_libraries(
	        sol::lib::base, sol::lib::coroutine, sol::lib::table, sol::lib::math, sol::lib::string, sol::lib::ffi);
	InitLuaAPI(state, engine);
	return state;
}

void axm::lua::InitLuaAPI(sol::state& state, AxiomEngine& engine)
{
	g_Context.m_Engine = &engine;
	g_Context.m_VK     = &engine.m_GPU->m_VK;

	state.new_usertype<aml::Float2>(
	        "vec2",
	        sol::call_constructor,
	        sol::factories([]() { return aml::Float2(); }, [](const f32 x, const f32 y) { return aml::Float2(x, y); }),
	        "x",
	        &aml::Float2::x,
	        "y",
	        &aml::Float2::y);

	state.new_usertype<aml::Vec3>(
	        "vec3",
	        sol::call_constructor,
	        sol::factories([](const f32 v) { return aml::Vec3(v, v, v); },
	                       [](const f32 x, const f32 y, const f32 z) { return aml::Vec3(x, y, z); }),
	        "x",
	        sol::property(&aml::Vec3::GetX, &aml::Vec3::SetX),
	        "y",
	        sol::property(&aml::Vec3::GetY, &aml::Vec3::SetY),
	        "z",
	        sol::property(&aml::Vec3::GetZ, &aml::Vec3::SetZ),
	        "normalized",
	        &aml::Vec3::Normalized,
	        "length",
	        &aml::Vec3::Length,
	        sol::meta_function::addition,
	        [](const aml::Vec3& l, const aml::Vec3& r) { return l + r; },
	        sol::meta_function::subtraction,
	        [](const aml::Vec3& l, const aml::Vec3& r) { return l - r; },
	        sol::meta_function::multiplication,
	        [](const aml::Vec3& l, const aml::Vec3& r) { return l * r; },
	        sol::meta_function::division,
	        [](const aml::Vec3& l, const aml::Vec3& r) { return l / r; });

	state.new_usertype<Transform>("Transform",
	                              sol::call_constructor,
	                              sol::factories([]() { return Transform(); }),
	                              "position",
	                              &Transform::m_Position,
	                              "euler",
	                              &Transform::m_Euler,
	                              "scale",
	                              &Transform::m_Scale,
	                              "forward",
	                              sol::readonly(&Transform::m_Forward),
	                              "right",
	                              sol::readonly(&Transform::m_Right),
	                              "up",
	                              sol::readonly(&Transform::m_Up),
	                              "update_direction_vectors",
	                              &Transform::UpdateDirectionVectors);

	state.new_usertype<Camera>("Camera",
	                           sol::call_constructor,
	                           sol::factories([]() { return Camera(); }),
	                           "transform",
	                           sol::property([](Camera& c) -> Transform& { return c.m_Transform; }),
	                           "viewport_dimensions",
	                           sol::property([](Camera& c) { return c.m_ViewportDimensions; },
	                                         [](Camera& c, const aml::Float2& v) { c.m_ViewportDimensions = v; }),
	                           "fov",
	                           &Camera::m_FOV,
	                           "near_plane",
	                           &Camera::m_NearPlane,
	                           "far_plane",
	                           &Camera::m_FarPlane);

	state.new_usertype<FlyCamController>("FlyCamController",
	                                     sol::no_constructor,
	                                     "movement_speed",
	                                     &FlyCamController::m_MovementSpeed,
	                                     "rotation_speed",
	                                     &FlyCamController::m_RotationSpeed,
	                                     "update",
	                                     &FlyCamController::Update);

	state.new_usertype<Shader>(
	        "Shader",
	        sol::no_constructor,
	        "valid",
	        &Shader::Valid,
	        "reload",
	        [](Shader& s) { return s.Reload(*g_Context.m_VK); },
	        "free",
	        [](Shader& s) { s.Free(*g_Context.m_VK); });

	state.new_usertype<vku::VertexDescription>("VertexDescription", sol::no_constructor);

	state.new_usertype<vku::RasterizationState>("RasterizationState",
	                                            sol::no_constructor,
	                                            "polygon_mode",
	                                            &vku::RasterizationState::m_PolygonMode,
	                                            "cull_mode",
	                                            &vku::RasterizationState::m_CullMode,
	                                            "enable_msaa",
	                                            &vku::RasterizationState::m_EnableMSAA,
	                                            "depth_compare",
	                                            &vku::RasterizationState::m_DepthCompareOp,
	                                            "topology",
	                                            &vku::RasterizationState::m_InputAssemblyTopology,
	                                            "front_face",
	                                            &vku::RasterizationState::m_FrontFace,
	                                            "line_width",
	                                            &vku::RasterizationState::m_LineWidth);

	state.new_usertype<LuaPipeline>(
	        "Pipeline", sol::no_constructor, "free", [](LuaPipeline& p) { p.m_Data.Free(*g_Context.m_VK); });

	state.new_usertype<Mesh>("Mesh",
	                         sol::no_constructor,
	                         "index_count",
	                         &Mesh::m_IndexCount,
	                         "valid",
	                         &Mesh::m_Valid,
	                         "free",
	                         [](Mesh& m) { m.Free(*g_Context.m_VK); });

	state.new_usertype<LuaUniformBlock>("UniformBlock",
	                                    sol::no_constructor,
	                                    sol::meta_function::new_index,
	                                    [](LuaUniformBlock& block, sol::object key, sol::object value)
	                                    {
		                                    if (!key.is<std::string>())
		                                    {
			                                    return;
		                                    }
		                                    WriteUniform(block, key.as<std::string>(), value);
	                                    });

	state.new_usertype<LuaMaterial>(
	        "Material",
	        sol::no_constructor,
	        "create_buffer",
	        [](LuaMaterial& m, u32 set, u32 binding) { m.m_Material.CreateBuffer(*g_Context.m_VK, set, binding); },
	        "uniforms",
	        [](LuaMaterial& m, u32 set, u32 binding) { return LuaUniformBlock{&m, set, binding}; },
	        "set_sampler",
	        [](LuaMaterial& m, const char* name, LuaTexture& texture)
	        {
		        if (texture.m_Texture == nullptr)
		        {
			        return false;
		        }
		        return m.m_Material.SetSampler(*g_Context.m_VK,
		                                       name,
		                                       texture.m_Texture->View(),
		                                       g_Context.m_Engine->m_GPU->m_Samplers.m_NearestWrapSampler);
	        },
	        "free",
	        [](LuaMaterial& m) { m.m_Material.Free(*g_Context.m_VK); });

	state.new_usertype<LuaTexture>(
	        "Texture",
	        sol::no_constructor,
	        "valid",
	        [](LuaTexture& t) { return t.m_Texture != nullptr; },
	        "view",
	        [](LuaTexture& t) { return t.m_Texture == nullptr ? u64(0) : reinterpret_cast<u64>(t.m_Texture->View()); },
	        "sampler",
	        [](LuaTexture& t)
	        { return t.m_Texture == nullptr ? u64(0) : reinterpret_cast<u64>(t.m_Texture->Sampler()); });

	state.new_usertype<LuaModel>(
	        "Model",
	        sol::no_constructor,
	        "mesh_count",
	        [](LuaModel& m) { return m.m_Asset == nullptr ? size_t(0) : m.m_Asset->m_Data.m_Meshes.size(); },
	        "mesh",
	        [](LuaModel& m, size_t index)
	        {
		        if (m.m_Asset == nullptr || index < 1 || index > m.m_Asset->m_Data.m_Meshes.size())
		        {
			        return Mesh{};
		        }
		        return m.m_Asset->m_Data.m_Meshes[index - 1].m_Mesh;
	        },
	        "material_index",
	        [](LuaModel& m, size_t index)
	        {
		        if (m.m_Asset == nullptr || index < 1 || index > m.m_Asset->m_Data.m_Meshes.size())
		        {
			        return u32(0);
		        }
		        return m.m_Asset->m_Data.m_Meshes[index - 1].m_MaterialIndex;
	        },
	        "diffuse",
	        [](LuaModel& m, size_t index) { return LuaTexture{ResolveDiffuse(m.m_Asset, index)}; });

	state.new_usertype<NVGcolor>("nvg_color",
	                             sol::no_constructor,
	                             "r",
	                             sol::property([](NVGcolor& c) { return c.r; }, [](NVGcolor& c, f32 v) { c.r = v; }),
	                             "g",
	                             sol::property([](NVGcolor& c) { return c.g; }, [](NVGcolor& c, f32 v) { c.g = v; }),
	                             "b",
	                             sol::property([](NVGcolor& c) { return c.b; }, [](NVGcolor& c, f32 v) { c.b = v; }),
	                             "a",
	                             sol::property([](NVGcolor& c) { return c.a; }, [](NVGcolor& c, f32 v) { c.a = v; }));

	state.new_usertype<NVGpaint>("nvg_paint", sol::no_constructor);

	state.new_usertype<Im3d::Color>(
	        "im3d_color",
	        sol::call_constructor,
	        sol::factories([](f32 r, f32 g, f32 b, sol::optional<f32> a)
	                       { return Im3d::Color(r, g, b, a.value_or(1.0f)); }),
	        "r",
	        sol::property([](Im3d::Color& c) { return c.getR(); }, [](Im3d::Color& c, f32 v) { c.setR(v); }),
	        "g",
	        sol::property([](Im3d::Color& c) { return c.getG(); }, [](Im3d::Color& c, f32 v) { c.setG(v); }),
	        "b",
	        sol::property([](Im3d::Color& c) { return c.getB(); }, [](Im3d::Color& c, f32 v) { c.setB(v); }),
	        "a",
	        sol::property([](Im3d::Color& c) { return c.getA(); }, [](Im3d::Color& c, f32 v) { c.setA(v); }));

	sol::table axiom    = state.create_named_table("axiom");

	sol::table graphics = state.create_table();
	axiom["graphics"]   = graphics;
	graphics.set_function("create_shader",
	                      [](const char* vertPath, const char* fragPath)
	                      { return Shader(*g_Context.m_VK, String(vertPath), String(fragPath)); });
	graphics.set_function("create_compute_shader",
	                      [](const char* computePath) { return Shader(*g_Context.m_VK, String(computePath)); });
	graphics.set_function("pos_normal_uv_vertex_description",
	                      []() { return vertex::PosNormalUV::GetVertexDescription(*g_Context.m_VK); });
	graphics.set_function("default_raster_state", []() { return vku::defaults::DefaultRasterState; });
	graphics.set_function("cull_none_raster_state", []() { return vku::defaults::CullNoneRasterState; });
	graphics.set_function("create_raster_pipeline",
	                      [](Shader& shader, vku::VertexDescription& vertDesc, vku::RasterizationState& raster)
	                      {
		                      LuaPipeline pipeline;
		                      pipeline.m_Data =
		                              pipeline::CreateRasterPipeline(*g_Context.m_VK,
		                                                             shader,
		                                                             vertDesc,
		                                                             raster,
		                                                             g_Context.m_VK->m_SwapchainImageRenderPass,
		                                                             g_Context.m_VK->m_SwapChainImageExtent,
		                                                             1);
		                      return pipeline;
	                      });
	graphics.set_function("create_material", [](Shader& shader) { return CreateMaterial(shader.m_Program); });
	graphics.set_function("set_pipeline",
	                      [](LuaPipeline& pipeline)
	                      {
		                      g_Context.m_Layout = pipeline.m_Data.m_PipelineLayout;
		                      vkCmdBindPipeline(
		                              g_Context.m_Cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.m_Data.m_Pipeline);
	                      });
	graphics.set_function("draw",
	                      [](Mesh& mesh, LuaMaterial& material)
	                      {
		                      vkCmdBindDescriptorSets(
		                              g_Context.m_Cmd,
		                              VK_PIPELINE_BIND_POINT_GRAPHICS,
		                              g_Context.m_Layout,
		                              0,
		                              1,
		                              &material.m_Material.m_DescriptorSets[0].m_Sets[g_Context.m_Frame],
		                              0,
		                              nullptr);
		                      meshes::DrawMesh(g_Context.m_Viewport, mesh, g_Context.m_Cmd);
	                      });

	sol::table shapes = state.create_table();
	axiom["shapes"]   = shapes;
	shapes.set_function("cube",
	                    []()
	                    {
		                    const shapes::Description cube = shapes::GetCubeShape();
		                    return meshes::CreateMeshFromData(*g_Context.m_VK,
		                                                      cube.m_VertexBuffer,
		                                                      cube.m_BufferLength * sizeof(f32),
		                                                      cube.m_IndexBuffer,
		                                                      cube.m_NumIndices,
		                                                      "Cube");
	                    });

	sol::table assets = state.create_table();
	axiom["assets"]   = assets;
	assets.set_function("load_model",
	                    [](const char* path)
	                    {
		                    AssetHandle handle = g_Context.m_Engine->m_AssetManager.LoadAsset(Filesystem::path(path),
		                                                                                      AssetType::Model);
		                    g_Context.m_Engine->m_AssetManager.WaitAllAssets();
		                    Asset* asset = g_Context.m_Engine->m_AssetManager.GetAsset(handle);
		                    return LuaModel{dynamic_cast<ModelAsset*>(asset)};
	                    });
	assets.set_function("load_texture", [](const char* path) { return LoadTextureSync(path); });

	sol::table window = state.create_table();
	axiom["window"]   = window;
	window.set_function("width", []() { return g_Context.m_Engine->m_Window.m_Width; });
	window.set_function("height", []() { return g_Context.m_Engine->m_Window.m_Height; });
	window.set_function("grab_cursor", []() { return g_Context.m_Engine->m_Window.GrabCursor(); });
	window.set_function("release_cursor", []() { return g_Context.m_Engine->m_Window.ReleaseCursor(); });

	sol::table axmath = state.create_table();
	axiom["math"]     = axmath;
	axmath.set_function("mvp",
	                    [](Transform& transform, Camera& camera)
	                    {
		                    const aml::Mat44 mvp = camera.GetViewProjectionMatrix() * transform.GetModelMatrix();
		                    std::vector<f32> values;
		                    values.reserve(16);
		                    for (u32 col = 0; col < 4u; ++col)
		                    {
			                    const aml::Vec4 column = mvp.GetColumn4(col);
			                    values.push_back(column.GetX());
			                    values.push_back(column.GetY());
			                    values.push_back(column.GetZ());
			                    values.push_back(column.GetW());
		                    }
		                    return values;
	                    });
	axmath.set_function("view_projection",
	                    [](Camera& camera)
	                    {
		                    const aml::Mat44 mvp = camera.GetViewProjectionMatrix();
		                    std::vector<f32> values;
		                    values.reserve(16);
		                    for (u32 col = 0; col < 4u; ++col)
		                    {
			                    const aml::Vec4 column = mvp.GetColumn4(col);
			                    values.push_back(column.GetX());
			                    values.push_back(column.GetY());
			                    values.push_back(column.GetZ());
			                    values.push_back(column.GetW());
		                    }
		                    return values;
	                    });
	axmath.set_function("look_at",
	                    [](const aml::Vec3& position, const aml::Vec3& target, const aml::Vec3& up)
	                    {
		                    const glm::mat4 view =
		                            glm::lookAt(glm::vec3(position.GetX(), position.GetY(), position.GetZ()),
		                                        glm::vec3(target.GetX(), target.GetY(), target.GetZ()),
		                                        glm::vec3(up.GetX(), up.GetY(), up.GetZ()));
		                    std::vector<f32> values;
		                    values.reserve(16);
		                    for (u32 col = 0; col < 4u; ++col)
		                    {
			                    for (u32 row = 0; row < 4u; ++row)
			                    {
				                    values.push_back(view[col][row]);
			                    }
		                    }
		                    return values;
	                    });

	sol::table input = state.create_table();
	axiom["input"]   = input;
	input.set_function("key_down", [](i32 key) { return g_Context.m_Engine->m_Input.IsKeyDown(CAST(key, Keycode)); });
	input.set_function("key_just_pressed",
	                   [](i32 key) { return g_Context.m_Engine->m_Input.IsKeyJustPressed(CAST(key, Keycode)); });
	input.set_function("mouse_down",
	                   [](i32 button)
	                   { return g_Context.m_Engine->m_Input.IsMouseButtonDown(CAST(button, MouseButton)); });
	input.set_function("mouse_just_pressed",
	                   [](i32 button)
	                   { return g_Context.m_Engine->m_Input.IsMouseButtonJustPressed(CAST(button, MouseButton)); });
	input.set_function("mouse_position", []() { return g_Context.m_Engine->m_Input.GetMousePosition(); });
	input.set_function("mouse_last_position", []() { return g_Context.m_Engine->m_Input.GetMousePositionLastFrame(); });
	input.set_function("mouse_velocity",
	                   []()
	                   {
		                   return g_Context.m_Engine->m_Input.GetMouseVelocity(
		                           aml::Float2(CAST(g_Context.m_Engine->m_Window.m_Width, f32),
		                                       CAST(g_Context.m_Engine->m_Window.m_Height, f32)));
	                   });
	input.set_function("gamepad_button_down",
	                   [](u8 index, i32 button)
	                   { return g_Context.m_Engine->m_Input.IsGamepadButtonDown(index, CAST(button, GamepadButton)); });
	input.set_function(
	        "gamepad_button_just_pressed",
	        [](u8 index, i32 button)
	        { return g_Context.m_Engine->m_Input.IsGamepadButtonJustPressed(index, CAST(button, GamepadButton)); });
	input.set_function("gamepad_axis",
	                   [](u8 index, i32 axis)
	                   { return g_Context.m_Engine->m_Input.GetGamepadAxis(index, CAST(axis, GamepadAxis)); });

	sol::table keys = state.create_table();
	axiom["key"]    = keys;
	auto bindKey    = [&keys](const char* name, Keycode code) { keys[name] = CAST(code, i32); };
	bindKey("unknown", Keycode::UNKNOWN);
	bindKey("return", Keycode::RETURN);
	bindKey("escape", Keycode::ESCAPE);
	bindKey("backspace", Keycode::BACKSPACE);
	bindKey("tab", Keycode::TAB);
	bindKey("space", Keycode::SPACE);
	bindKey("exclaim", Keycode::EXCLAIM);
	bindKey("double_apostrophe", Keycode::DBLAPOSTROPHE);
	bindKey("hash", Keycode::HASH);
	bindKey("dollar", Keycode::DOLLAR);
	bindKey("percent", Keycode::PERCENT);
	bindKey("ampersand", Keycode::AMPERSAND);
	bindKey("apostrophe", Keycode::APOSTROPHE);
	bindKey("left_paren", Keycode::LEFTPAREN);
	bindKey("right_paren", Keycode::RIGHTPAREN);
	bindKey("asterisk", Keycode::ASTERISK);
	bindKey("plus", Keycode::PLUS);
	bindKey("comma", Keycode::COMMA);
	bindKey("minus", Keycode::MINUS);
	bindKey("period", Keycode::PERIOD);
	bindKey("slash", Keycode::SLASH);
	bindKey("colon", Keycode::COLON);
	bindKey("semicolon", Keycode::SEMICOLON);
	bindKey("less", Keycode::LESS);
	bindKey("equals", Keycode::EQUALS);
	bindKey("greater", Keycode::GREATER);
	bindKey("question", Keycode::QUESTION);
	bindKey("at", Keycode::AT);
	bindKey("left_bracket", Keycode::LEFTBRACKET);
	bindKey("backslash", Keycode::BACKSLASH);
	bindKey("right_bracket", Keycode::RIGHTBRACKET);
	bindKey("caret", Keycode::CARET);
	bindKey("underscore", Keycode::UNDERSCORE);
	bindKey("grave", Keycode::GRAVE);
	bindKey("left_brace", Keycode::LEFTBRACE);
	bindKey("pipe", Keycode::PIPE);
	bindKey("right_brace", Keycode::RIGHTBRACE);
	bindKey("tilde", Keycode::TILDE);
	bindKey("delete", Keycode::DELETE);
	bindKey("plus_minus", Keycode::PLUSMINUS);
	bindKey("caps_lock", Keycode::CAPSLOCK);
	bindKey("print_screen", Keycode::PRINTSCREEN);
	bindKey("scroll_lock", Keycode::SCROLLLOCK);
	bindKey("pause", Keycode::PAUSE);
	bindKey("insert", Keycode::INSERT);
	bindKey("home", Keycode::HOME);
	bindKey("page_up", Keycode::PAGEUP);
	bindKey("end", Keycode::END);
	bindKey("page_down", Keycode::PAGEDOWN);
	bindKey("right", Keycode::RIGHT);
	bindKey("left", Keycode::LEFT);
	bindKey("down", Keycode::DOWN);
	bindKey("up", Keycode::UP);
	bindKey("num_lock", Keycode::NUMLOCKCLEAR);
	bindKey("kp_divide", Keycode::KP_DIVIDE);
	bindKey("kp_multiply", Keycode::KP_MULTIPLY);
	bindKey("kp_minus", Keycode::KP_MINUS);
	bindKey("kp_plus", Keycode::KP_PLUS);
	bindKey("kp_enter", Keycode::KP_ENTER);
	bindKey("kp_period", Keycode::KP_PERIOD);
	bindKey("application", Keycode::APPLICATION);
	bindKey("power", Keycode::POWER);
	bindKey("kp_equals", Keycode::KP_EQUALS);
	for (char c = 'a'; c <= 'z'; ++c)
	{
		bindKey(std::string(1, c).c_str(), static_cast<Keycode>(c));
	}
	for (i32 i = 0; i < 10; ++i)
	{
		bindKey(("num" + std::to_string(i)).c_str(), static_cast<Keycode>('0' + i));
	}
	bindKey("kp0", Keycode::KP_0);
	bindKey("kp1", Keycode::KP_1);
	bindKey("kp2", Keycode::KP_2);
	bindKey("kp3", Keycode::KP_3);
	bindKey("kp4", Keycode::KP_4);
	bindKey("kp5", Keycode::KP_5);
	bindKey("kp6", Keycode::KP_6);
	bindKey("kp7", Keycode::KP_7);
	bindKey("kp8", Keycode::KP_8);
	bindKey("kp9", Keycode::KP_9);
	bindKey("f1", Keycode::F1);
	bindKey("f2", Keycode::F2);
	bindKey("f3", Keycode::F3);
	bindKey("f4", Keycode::F4);
	bindKey("f5", Keycode::F5);
	bindKey("f6", Keycode::F6);
	bindKey("f7", Keycode::F7);
	bindKey("f8", Keycode::F8);
	bindKey("f9", Keycode::F9);
	bindKey("f10", Keycode::F10);
	bindKey("f11", Keycode::F11);
	bindKey("f12", Keycode::F12);

	sol::table mouse = state.create_table();
	axiom["mouse"]   = mouse;
	auto bindMouse   = [&mouse](const char* name, MouseButton button) { mouse[name] = CAST(button, i32); };
	bindMouse("left", MouseButton::Left);
	bindMouse("middle", MouseButton::Middle);
	bindMouse("right", MouseButton::Right);
	bindMouse("extra1", MouseButton::Extra1);
	bindMouse("extra2", MouseButton::Extra2);

	sol::table gamepad        = state.create_table();
	axiom["gamepad"]          = gamepad;
	sol::table gamepadButtons = state.create_table();
	gamepad["button"]         = gamepadButtons;
	auto bindGamepadButton    = [&gamepadButtons](const char* name, GamepadButton button)
	{ gamepadButtons[name] = CAST(button, i32); };
	bindGamepadButton("face_north", GamepadButton::FaceNorth);
	bindGamepadButton("face_south", GamepadButton::FaceSouth);
	bindGamepadButton("face_east", GamepadButton::FaceEast);
	bindGamepadButton("face_west", GamepadButton::FaceWest);
	bindGamepadButton("dpad_up", GamepadButton::DpadUp);
	bindGamepadButton("dpad_down", GamepadButton::DpadDown);
	bindGamepadButton("dpad_left", GamepadButton::DpadLeft);
	bindGamepadButton("dpad_right", GamepadButton::DpadRight);
	bindGamepadButton("right_bumper", GamepadButton::RightBumper);
	bindGamepadButton("left_bumper", GamepadButton::LeftBumper);
	bindGamepadButton("start", GamepadButton::Start);
	bindGamepadButton("back", GamepadButton::Back);
	bindGamepadButton("home", GamepadButton::Home);
	sol::table gamepadAxes = state.create_table();
	gamepad["axis"]        = gamepadAxes;
	auto bindGamepadAxis = [&gamepadAxes](const char* name, GamepadAxis axis) { gamepadAxes[name] = CAST(axis, i32); };
	bindGamepadAxis("left_stick_x", GamepadAxis::LeftStickX);
	bindGamepadAxis("left_stick_y", GamepadAxis::LeftStickY);
	bindGamepadAxis("right_stick_x", GamepadAxis::RightStickX);
	bindGamepadAxis("right_stick_y", GamepadAxis::RightStickY);
	bindGamepadAxis("left_trigger", GamepadAxis::LeftTrigger);
	bindGamepadAxis("right_trigger", GamepadAxis::RightTrigger);

	sol::table textureMap      = state.create_table();
	axiom["texture_map"]       = textureMap;
	textureMap["unknown"]      = CAST(TextureMapType::Unknown, i32);
	textureMap["diffuse"]      = CAST(TextureMapType::Diffuse, i32);
	textureMap["normal"]       = CAST(TextureMapType::Normal, i32);
	textureMap["specular"]     = CAST(TextureMapType::Specular, i32);
	textureMap["metallic"]     = CAST(TextureMapType::Metallic, i32);
	textureMap["opacity"]      = CAST(TextureMapType::Opacity, i32);
	textureMap["height"]       = CAST(TextureMapType::Height, i32);
	textureMap["displacement"] = CAST(TextureMapType::Displacement, i32);
	textureMap["roughness"]    = CAST(TextureMapType::Roughness, i32);
	textureMap["ao"]           = CAST(TextureMapType::AO, i32);
	textureMap["emissive"]     = CAST(TextureMapType::Emissive, i32);

	sol::table assetTypes      = state.create_table();
	axiom["asset_type"]        = assetTypes;
	assetTypes["unknown"]      = CAST(AssetType::Unknown, i32);
	assetTypes["model"]        = CAST(AssetType::Model, i32);
	assetTypes["texture"]      = CAST(AssetType::Texture, i32);
	assetTypes["shader"]       = CAST(AssetType::Shader, i32);
	assetTypes["audio"]        = CAST(AssetType::Audio, i32);
	assetTypes["text"]         = CAST(AssetType::Text, i32);
	assetTypes["binary"]       = CAST(AssetType::Binary, i32);

	sol::table assetStates     = state.create_table();
	axiom["asset_state"]       = assetStates;
	assetStates["not_loaded"]  = CAST(AssetState::NotLoaded, i32);
	assetStates["loading"]     = CAST(AssetState::Loading, i32);
	assetStates["loaded"]      = CAST(AssetState::Loaded, i32);
	assetStates["unloading"]   = CAST(AssetState::Unloading, i32);

	sol::table imgui           = state.create_table();
	axiom["imgui"]             = imgui;
	imgui.set_function("begin", [](const char* name) { return ImGui::Begin(name); });
	imgui.set_function("begin_flags", [](const char* name, i32 flags) { return ImGui::Begin(name, nullptr, flags); });
	imgui.set_function("finish", []() { ImGui::End(); });
	imgui.set_function(
	        "begin_child",
	        sol::overload([](const char* name) { return ImGui::BeginChild(name); },
	                      [](const char* name, f32 w, f32 h) { return ImGui::BeginChild(name, ImVec2(w, h)); }));
	imgui.set_function("finish_child", []() { ImGui::EndChild(); });
	imgui.set_function("text", [](const char* text) { ImGui::TextUnformatted(text); });
	imgui.set_function("text_colored",
	                   [](f32 r, f32 g, f32 b, f32 a, const char* text)
	                   { ImGui::TextColored(ImVec4(r, g, b, a), "%s", text); });
	imgui.set_function("text_disabled", [](const char* text) { ImGui::TextDisabled("%s", text); });
	imgui.set_function("text_wrapped", [](const char* text) { ImGui::TextWrapped("%s", text); });
	imgui.set_function("label_text", [](const char* label, const char* text) { ImGui::LabelText(label, "%s", text); });
	imgui.set_function("bullet_text", [](const char* text) { ImGui::BulletText("%s", text); });
	imgui.set_function("separator", []() { ImGui::Separator(); });
	imgui.set_function("separator_text", [](const char* text) { ImGui::SeparatorText(text); });
	imgui.set_function("same_line", []() { ImGui::SameLine(); });
	imgui.set_function("new_line", []() { ImGui::NewLine(); });
	imgui.set_function("spacing", []() { ImGui::Spacing(); });
	imgui.set_function("dummy", [](f32 w, f32 h) { ImGui::Dummy(ImVec2(w, h)); });
	imgui.set_function("button", [](const char* label) { return ImGui::Button(label); });
	imgui.set_function("small_button", [](const char* label) { return ImGui::SmallButton(label); });
	imgui.set_function("invisible_button",
	                   [](const char* label, f32 w, f32 h) { return ImGui::InvisibleButton(label, ImVec2(w, h)); });
	imgui.set_function("arrow_button",
	                   [](const char* label, i32 dir) { return ImGui::ArrowButton(label, CAST(dir, ImGuiDir)); });
	imgui.set_function("checkbox",
	                   [](const char* label, bool value)
	                   {
		                   bool       v       = value;
		                   const bool changed = ImGui::Checkbox(label, &v);
		                   return std::make_tuple(changed, v);
	                   });
	imgui.set_function("radio_button",
	                   [](const char* label, bool active) { return ImGui::RadioButton(label, active); });
	imgui.set_function("slider_float",
	                   [](const char* label, f32 value, f32 min, f32 max)
	                   {
		                   ImGui::SliderFloat(label, &value, min, max);
		                   return value;
	                   });
	imgui.set_function("slider_int",
	                   [](const char* label, i32 value, i32 min, i32 max)
	                   {
		                   ImGui::SliderInt(label, &value, min, max);
		                   return value;
	                   });
	imgui.set_function("drag_float",
	                   [](const char* label, f32 value)
	                   {
		                   ImGui::DragFloat(label, &value);
		                   return value;
	                   });
	imgui.set_function("drag_float3",
	                   [](const char* label, f32 x, f32 y, f32 z)
	                   {
		                   float v[3] = {x, y, z};
		                   ImGui::DragFloat3(label, v);
		                   return std::make_tuple(v[0], v[1], v[2]);
	                   });
	imgui.set_function("drag_int",
	                   [](const char* label, i32 value)
	                   {
		                   ImGui::DragInt(label, &value);
		                   return value;
	                   });
	imgui.set_function("input_float",
	                   [](const char* label, f32 value)
	                   {
		                   ImGui::InputFloat(label, &value);
		                   return value;
	                   });
	imgui.set_function("input_int",
	                   [](const char* label, i32 value)
	                   {
		                   ImGui::InputInt(label, &value);
		                   return value;
	                   });
	imgui.set_function("input_text",
	                   [](const char* label, const char* value)
	                   {
		                   auto& buffer = InputBuffers()[label];
		                   if (buffer[0] == '\0' && value != nullptr)
		                   {
			                   std::snprintf(buffer.data(), buffer.size(), "%s", value);
		                   }
		                   ImGui::InputText(label, buffer.data(), buffer.size());
		                   return std::string(buffer.data());
	                   });
	imgui.set_function("color_edit3",
	                   [](const char* label, f32 r, f32 g, f32 b)
	                   {
		                   float v[3] = {r, g, b};
		                   ImGui::ColorEdit3(label, v);
		                   return std::make_tuple(v[0], v[1], v[2]);
	                   });
	imgui.set_function("color_edit4",
	                   [](const char* label, f32 r, f32 g, f32 b, f32 a)
	                   {
		                   float v[4] = {r, g, b, a};
		                   ImGui::ColorEdit4(label, v);
		                   return std::make_tuple(v[0], v[1], v[2], v[3]);
	                   });
	imgui.set_function("tree_node", [](const char* label) { return ImGui::TreeNode(label); });
	imgui.set_function("tree_node_ex", [](const char* label, i32 flags) { return ImGui::TreeNodeEx(label, flags); });
	imgui.set_function("tree_pop", []() { ImGui::TreePop(); });
	imgui.set_function("collapsing_header", [](const char* label) { return ImGui::CollapsingHeader(label); });
	imgui.set_function("begin_combo",
	                   [](const char* label, const char* preview) { return ImGui::BeginCombo(label, preview); });
	imgui.set_function("finish_combo", []() { ImGui::EndCombo(); });
	imgui.set_function("combo",
	                   [](const char* label, i32 current, sol::table items)
	                   {
		                   std::vector<std::string> storage;
		                   std::vector<const char*> pointers;
		                   const size_t             count = items.size();
		                   storage.reserve(count);
		                   pointers.reserve(count);
		                   for (size_t i = 1; i <= count; ++i)
		                   {
			                   storage.push_back(items.get<std::string>(i));
		                   }
		                   for (auto& entry : storage)
		                   {
			                   pointers.push_back(entry.c_str());
		                   }
		                   ImGui::Combo(label, &current, pointers.data(), CAST(pointers.size(), int));
		                   return current;
	                   });
	imgui.set_function("list_box",
	                   [](const char* label, i32 current, sol::table items)
	                   {
		                   std::vector<std::string> storage;
		                   std::vector<const char*> pointers;
		                   const size_t             count = items.size();
		                   storage.reserve(count);
		                   pointers.reserve(count);
		                   for (size_t i = 1; i <= count; ++i)
		                   {
			                   storage.push_back(items.get<std::string>(i));
		                   }
		                   for (auto& entry : storage)
		                   {
			                   pointers.push_back(entry.c_str());
		                   }
		                   ImGui::ListBox(label, &current, pointers.data(), CAST(pointers.size(), int));
		                   return current;
	                   });
	imgui.set_function(
	        "selectable",
	        sol::overload([](const char* label) { return ImGui::Selectable(label); },
	                      [](const char* label, bool selected) { return ImGui::Selectable(label, selected); }));
	imgui.set_function("progress_bar", [](f32 fraction) { ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f), nullptr); });
	imgui.set_function("progress_bar_overlay",
	                   [](f32 fraction, const char* overlay)
	                   { ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f), overlay); });
	imgui.set_function("plot_lines",
	                   [](const char* label, sol::table values)
	                   {
		                   std::vector<float> data;
		                   const size_t       count = values.size();
		                   data.reserve(count);
		                   for (size_t i = 1; i <= count; ++i)
		                   {
			                   data.push_back(values.get_or(CAST(i, int), f32(0)));
		                   }
		                   ImGui::PlotLines(label, data.data(), CAST(data.size(), int));
	                   });
	imgui.set_function("plot_histogram",
	                   [](const char* label, sol::table values)
	                   {
		                   std::vector<float> data;
		                   const size_t       count = values.size();
		                   data.reserve(count);
		                   for (size_t i = 1; i <= count; ++i)
		                   {
			                   data.push_back(values.get_or(CAST(i, int), f32(0)));
		                   }
		                   ImGui::PlotHistogram(label, data.data(), CAST(data.size(), int));
	                   });
	imgui.set_function("begin_main_menu_bar", []() { return ImGui::BeginMainMenuBar(); });
	imgui.set_function("finish_main_menu_bar", []() { ImGui::EndMainMenuBar(); });
	imgui.set_function("begin_menu_bar", []() { return ImGui::BeginMenuBar(); });
	imgui.set_function("finish_menu_bar", []() { ImGui::EndMenuBar(); });
	imgui.set_function("begin_menu", [](const char* label) { return ImGui::BeginMenu(label); });
	imgui.set_function("finish_menu", []() { ImGui::EndMenu(); });
	imgui.set_function(
	        "menu_item",
	        sol::overload([](const char* label) { return ImGui::MenuItem(label); },
	                      [](const char* label, const char* shortcut) { return ImGui::MenuItem(label, shortcut); }));
	imgui.set_function("push_id", [](const char* id) { ImGui::PushID(id); });
	imgui.set_function("pop_id", []() { ImGui::PopID(); });
	imgui.set_function("push_item_width", [](f32 width) { ImGui::PushItemWidth(width); });
	imgui.set_function("pop_item_width", []() { ImGui::PopItemWidth(); });
	imgui.set_function("set_next_item_width", [](f32 width) { ImGui::SetNextItemWidth(width); });
	imgui.set_function("push_style_color",
	                   [](i32 index, f32 r, f32 g, f32 b, f32 a)
	                   { ImGui::PushStyleColor(CAST(index, ImGuiCol), ImVec4(r, g, b, a)); });
	imgui.set_function("pop_style_color", [](i32 count) { ImGui::PopStyleColor(count); });
	imgui.set_function("push_style_var_float",
	                   [](i32 index, f32 value) { ImGui::PushStyleVar(CAST(index, ImGuiStyleVar), value); });
	imgui.set_function("push_style_var_vec2",
	                   [](i32 index, f32 x, f32 y) { ImGui::PushStyleVar(CAST(index, ImGuiStyleVar), ImVec2(x, y)); });
	imgui.set_function("pop_style_var", [](i32 count) { ImGui::PopStyleVar(count); });
	imgui.set_function("begin_tooltip", []() { return ImGui::BeginTooltip(); });
	imgui.set_function("finish_tooltip", []() { ImGui::EndTooltip(); });
	imgui.set_function("set_tooltip", [](const char* text) { ImGui::SetTooltip("%s", text); });
	imgui.set_function("is_item_hovered", []() { return ImGui::IsItemHovered(); });
	imgui.set_function("is_item_clicked", []() { return ImGui::IsItemClicked(); });
	imgui.set_function("get_window_size",
	                   []()
	                   {
		                   const ImVec2 s = ImGui::GetWindowSize();
		                   return std::make_tuple(s.x, s.y);
	                   });
	imgui.set_function("get_content_region_avail",
	                   []()
	                   {
		                   const ImVec2 s = ImGui::GetContentRegionAvail();
		                   return std::make_tuple(s.x, s.y);
	                   });
	imgui.set_function("calc_text_size",
	                   [](const char* text)
	                   {
		                   const ImVec2 s = ImGui::CalcTextSize(text);
		                   return std::make_tuple(s.x, s.y);
	                   });
	imgui.set_function("set_next_window_pos", [](f32 x, f32 y) { ImGui::SetNextWindowPos(ImVec2(x, y)); });
	imgui.set_function("set_next_window_size", [](f32 w, f32 h) { ImGui::SetNextWindowSize(ImVec2(w, h)); });
	imgui.set_function("set_next_window_bg_alpha", [](f32 alpha) { ImGui::SetNextWindowBgAlpha(alpha); });
	imgui.set_function("framerate", []() { return ImGui::GetIO().Framerate; });
	imgui.set_function("delta_time", []() { return ImGui::GetIO().DeltaTime; });
	imgui.set_function("transform_edit", [](Transform& t) { return ImGuiEx::TransformEdit(t); });
	imgui.set_function("camera_edit", [](Camera& c) { return ImGuiEx::CameraEdit(c); });
	imgui.set_function("flycam_edit", [](FlyCamController& f) { return ImGuiEx::FlyCamControllerEdit(f); });

#define AXM_IMGUI_ENUM(table, prefix, name) (table)[#name] = CAST(prefix##name, i32)

	sol::table imguiCol                                    = state.create_table();
	imgui["col"]                                           = imguiCol;
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, Text);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TextDisabled);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, WindowBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ChildBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, PopupBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, Border);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, BorderShadow);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, FrameBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, FrameBgHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, FrameBgActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TitleBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TitleBgActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TitleBgCollapsed);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, MenuBarBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ScrollbarBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ScrollbarGrab);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ScrollbarGrabHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ScrollbarGrabActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, CheckMark);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, SliderGrab);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, SliderGrabActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, Button);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ButtonHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ButtonActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, Header);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, HeaderHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, HeaderActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, Separator);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, SeparatorHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, SeparatorActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ResizeGrip);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ResizeGripHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ResizeGripActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, Tab);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TabHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TabActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TabUnfocused);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TabUnfocusedActive);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, DockingPreview);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, DockingEmptyBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, PlotLines);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, PlotLinesHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, PlotHistogram);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, PlotHistogramHovered);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TableHeaderBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TableBorderStrong);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TableBorderLight);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TableRowBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TableRowBgAlt);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, TextSelectedBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, DragDropTarget);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, NavHighlight);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, NavWindowingHighlight);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, NavWindowingDimBg);
	AXM_IMGUI_ENUM(imguiCol, ImGuiCol_, ModalWindowDimBg);

	sol::table imguiStyleVar = state.create_table();
	imgui["style_var"]       = imguiStyleVar;
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, Alpha);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, DisabledAlpha);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, WindowPadding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, WindowRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, WindowBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, WindowMinSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, WindowTitleAlign);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ChildRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ChildBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, PopupRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, PopupBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, FramePadding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, FrameRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, FrameBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ItemSpacing);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ItemInnerSpacing);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, IndentSpacing);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, CellPadding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ScrollbarSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ScrollbarRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, GrabMinSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, GrabRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, TabRounding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, TabBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, TabBarBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, TableAngledHeadersAngle);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, TableAngledHeadersTextAlign);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, ButtonTextAlign);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, SelectableTextAlign);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, SeparatorTextBorderSize);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, SeparatorTextAlign);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, SeparatorTextPadding);
	AXM_IMGUI_ENUM(imguiStyleVar, ImGuiStyleVar_, DockingSeparatorSize);

	sol::table imguiWindowFlags = state.create_table();
	imgui["window_flags"]       = imguiWindowFlags;
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, None);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoTitleBar);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoResize);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoMove);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoScrollbar);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoScrollWithMouse);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoCollapse);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, AlwaysAutoResize);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoBackground);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoSavedSettings);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoMouseInputs);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, MenuBar);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, HorizontalScrollbar);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoFocusOnAppearing);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoBringToFrontOnFocus);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, AlwaysVerticalScrollbar);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, AlwaysHorizontalScrollbar);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoNavInputs);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoNavFocus);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, UnsavedDocument);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoDocking);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoNav);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoDecoration);
	AXM_IMGUI_ENUM(imguiWindowFlags, ImGuiWindowFlags_, NoInputs);

	sol::table imguiTreeNodeFlags = state.create_table();
	imgui["tree_node_flags"]      = imguiTreeNodeFlags;
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, None);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, Selected);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, Framed);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, AllowOverlap);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, NoTreePushOnOpen);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, NoAutoOpenOnLog);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, DefaultOpen);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, OpenOnDoubleClick);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, OpenOnArrow);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, Leaf);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, Bullet);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, FramePadding);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, SpanAvailWidth);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, SpanFullWidth);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, SpanTextWidth);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, SpanAllColumns);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, NavLeftJumpsBackHere);
	AXM_IMGUI_ENUM(imguiTreeNodeFlags, ImGuiTreeNodeFlags_, CollapsingHeader);

	sol::table imguiInputTextFlags = state.create_table();
	imgui["input_text_flags"]      = imguiInputTextFlags;
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, None);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CharsDecimal);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CharsHexadecimal);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CharsUppercase);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CharsNoBlank);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, AutoSelectAll);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, EnterReturnsTrue);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, AllowTabInput);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CtrlEnterForNewLine);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, NoHorizontalScroll);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, AlwaysOverwrite);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, ReadOnly);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, Password);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, NoUndoRedo);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CharsScientific);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CallbackResize);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, CallbackEdit);
	AXM_IMGUI_ENUM(imguiInputTextFlags, ImGuiInputTextFlags_, EscapeClearsAll);

	sol::table imguiComboFlags = state.create_table();
	imgui["combo_flags"]       = imguiComboFlags;
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, None);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, PopupAlignLeft);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, HeightSmall);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, HeightRegular);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, HeightLarge);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, HeightLargest);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, NoArrowButton);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, NoPreview);
	AXM_IMGUI_ENUM(imguiComboFlags, ImGuiComboFlags_, WidthFitPreview);

	sol::table imguiSliderFlags = state.create_table();
	imgui["slider_flags"]       = imguiSliderFlags;
	AXM_IMGUI_ENUM(imguiSliderFlags, ImGuiSliderFlags_, None);
	AXM_IMGUI_ENUM(imguiSliderFlags, ImGuiSliderFlags_, AlwaysClamp);
	AXM_IMGUI_ENUM(imguiSliderFlags, ImGuiSliderFlags_, Logarithmic);
	AXM_IMGUI_ENUM(imguiSliderFlags, ImGuiSliderFlags_, NoRoundToFormat);
	AXM_IMGUI_ENUM(imguiSliderFlags, ImGuiSliderFlags_, NoInput);

	sol::table imguiColorEditFlags = state.create_table();
	imgui["color_edit_flags"]      = imguiColorEditFlags;
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, None);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoAlpha);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoPicker);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoOptions);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoSmallPreview);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoInputs);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoTooltip);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoLabel);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoSidePreview);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoDragDrop);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, NoBorder);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, AlphaBar);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, AlphaPreview);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, AlphaPreviewHalf);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, HDR);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, DisplayRGB);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, DisplayHSV);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, DisplayHex);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, Uint8);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, Float);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, PickerHueBar);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, PickerHueWheel);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, InputRGB);
	AXM_IMGUI_ENUM(imguiColorEditFlags, ImGuiColorEditFlags_, InputHSV);

#undef AXM_IMGUI_ENUM

	sol::table nvg = state.create_table();
	axiom["nvg"]   = nvg;
	auto ctx       = []() { return g_Context.m_Engine->m_GPU->m_VK.m_NanoVG; };

	nvg.set_function("rgb", [](u8 r, u8 g, u8 b) { return nvgRGB(r, g, b); });
	nvg.set_function("rgba", [](u8 r, u8 g, u8 b, u8 a) { return nvgRGBA(r, g, b, a); });
	nvg.set_function("rgb_f", [](f32 r, f32 g, f32 b) { return nvgRGBf(r, g, b); });
	nvg.set_function("rgba_f", [](f32 r, f32 g, f32 b, f32 a) { return nvgRGBAf(r, g, b, a); });
	nvg.set_function("hsl", [](f32 h, f32 s, f32 l) { return nvgHSL(h, s, l); });
	nvg.set_function("hsla", [](f32 h, f32 s, f32 l, u8 a) { return nvgHSLA(h, s, l, a); });
	nvg.set_function("lerp", [](NVGcolor& c0, NVGcolor& c1, f32 u) { return nvgLerpRGBA(c0, c1, u); });
	nvg.set_function("trans", [](NVGcolor& c, u8 a) { return nvgTransRGBA(c, a); });
	nvg.set_function("trans_f", [](NVGcolor& c, f32 a) { return nvgTransRGBAf(c, a); });

	nvg.set_function("linear_gradient",
	                 [ctx](f32 sx, f32 sy, f32 ex, f32 ey, NVGcolor& inner, NVGcolor& outer)
	                 { return nvgLinearGradient(ctx(), sx, sy, ex, ey, inner, outer); });
	nvg.set_function("box_gradient",
	                 [ctx](f32 x, f32 y, f32 w, f32 h, f32 radius, f32 feather, NVGcolor& inner, NVGcolor& outer)
	                 { return nvgBoxGradient(ctx(), x, y, w, h, radius, feather, inner, outer); });
	nvg.set_function("radial_gradient",
	                 [ctx](f32 cx, f32 cy, f32 inr, f32 outr, NVGcolor& inner, NVGcolor& outer)
	                 { return nvgRadialGradient(ctx(), cx, cy, inr, outr, inner, outer); });
	nvg.set_function("image_pattern",
	                 [ctx](f32 ox, f32 oy, f32 ex, f32 ey, f32 angle, i32 image, f32 alpha)
	                 { return nvgImagePattern(ctx(), ox, oy, ex, ey, angle, image, alpha); });

	nvg.set_function("begin_path", [ctx]() { nvgBeginPath(ctx()); });
	nvg.set_function("move_to", [ctx](f32 x, f32 y) { nvgMoveTo(ctx(), x, y); });
	nvg.set_function("line_to", [ctx](f32 x, f32 y) { nvgLineTo(ctx(), x, y); });
	nvg.set_function("bezier_to",
	                 [ctx](f32 c1x, f32 c1y, f32 c2x, f32 c2y, f32 x, f32 y)
	                 { nvgBezierTo(ctx(), c1x, c1y, c2x, c2y, x, y); });
	nvg.set_function("quad_to", [ctx](f32 cx, f32 cy, f32 x, f32 y) { nvgQuadTo(ctx(), cx, cy, x, y); });
	nvg.set_function("arc_to",
	                 [ctx](f32 x1, f32 y1, f32 x2, f32 y2, f32 radius) { nvgArcTo(ctx(), x1, y1, x2, y2, radius); });
	nvg.set_function("arc",
	                 [ctx](f32 cx, f32 cy, f32 r, f32 a0, f32 a1, i32 dir) { nvgArc(ctx(), cx, cy, r, a0, a1, dir); });
	nvg.set_function("close_path", [ctx]() { nvgClosePath(ctx()); });
	nvg.set_function("rect", [ctx](f32 x, f32 y, f32 w, f32 h) { nvgRect(ctx(), x, y, w, h); });
	nvg.set_function("rounded_rect",
	                 [ctx](f32 x, f32 y, f32 w, f32 h, f32 r) { nvgRoundedRect(ctx(), x, y, w, h, r); });
	nvg.set_function("rounded_rect_varying",
	                 [ctx](f32 x, f32 y, f32 w, f32 h, f32 tl, f32 tr, f32 br, f32 bl)
	                 { nvgRoundedRectVarying(ctx(), x, y, w, h, tl, tr, br, bl); });
	nvg.set_function("ellipse", [ctx](f32 cx, f32 cy, f32 rx, f32 ry) { nvgEllipse(ctx(), cx, cy, rx, ry); });
	nvg.set_function("circle", [ctx](f32 cx, f32 cy, f32 r) { nvgCircle(ctx(), cx, cy, r); });
	nvg.set_function("fill", [ctx]() { nvgFill(ctx()); });
	nvg.set_function("stroke", [ctx]() { nvgStroke(ctx()); });
	nvg.set_function("fill_color", [ctx](NVGcolor& color) { nvgFillColor(ctx(), color); });
	nvg.set_function("stroke_color", [ctx](NVGcolor& color) { nvgStrokeColor(ctx(), color); });
	nvg.set_function("fill_paint", [ctx](NVGpaint& paint) { nvgFillPaint(ctx(), paint); });
	nvg.set_function("stroke_paint", [ctx](NVGpaint& paint) { nvgStrokePaint(ctx(), paint); });
	nvg.set_function("path_winding", [ctx](i32 dir) { nvgPathWinding(ctx(), CAST(dir, NVGwinding)); });
	nvg.set_function("line_cap", [ctx](i32 cap) { nvgLineCap(ctx(), CAST(cap, NVGlineCap)); });
	nvg.set_function("line_join", [ctx](i32 join) { nvgLineJoin(ctx(), CAST(join, NVGlineCap)); });
	nvg.set_function("miter_limit", [ctx](f32 limit) { nvgMiterLimit(ctx(), limit); });
	nvg.set_function("stroke_width", [ctx](f32 width) { nvgStrokeWidth(ctx(), width); });
	nvg.set_function("shape_anti_alias", [ctx](bool enabled) { nvgShapeAntiAlias(ctx(), CAST(enabled, int)); });
	nvg.set_function("global_alpha", [ctx](f32 alpha) { nvgGlobalAlpha(ctx(), alpha); });
	nvg.set_function("global_composite_operation",
	                 [ctx](i32 op) { nvgGlobalCompositeOperation(ctx(), CAST(op, NVGcompositeOperation)); });
	nvg.set_function("global_composite_blend_func",
	                 [ctx](i32 src, i32 dst)
	                 { nvgGlobalCompositeBlendFunc(ctx(), CAST(src, NVGblendFactor), CAST(dst, NVGblendFactor)); });
	nvg.set_function("global_composite_blend_func_separate",
	                 [ctx](i32 srcRGB, i32 dstRGB, i32 srcA, i32 dstA)
	                 {
		                 nvgGlobalCompositeBlendFuncSeparate(ctx(),
		                                                     CAST(srcRGB, NVGblendFactor),
		                                                     CAST(dstRGB, NVGblendFactor),
		                                                     CAST(srcA, NVGblendFactor),
		                                                     CAST(dstA, NVGblendFactor));
	                 });
	nvg.set_function("save", [ctx]() { nvgSave(ctx()); });
	nvg.set_function("restore", [ctx]() { nvgRestore(ctx()); });
	nvg.set_function("reset", [ctx]() { nvgReset(ctx()); });
	nvg.set_function("translate", [ctx](f32 x, f32 y) { nvgTranslate(ctx(), x, y); });
	nvg.set_function("rotate", [ctx](f32 angle) { nvgRotate(ctx(), angle); });
	nvg.set_function("scale", [ctx](f32 x, f32 y) { nvgScale(ctx(), x, y); });
	nvg.set_function("skew_x", [ctx](f32 angle) { nvgSkewX(ctx(), angle); });
	nvg.set_function("skew_y", [ctx](f32 angle) { nvgSkewY(ctx(), angle); });
	nvg.set_function("transform",
	                 [ctx](f32 a, f32 b, f32 c, f32 d, f32 e, f32 f) { nvgTransform(ctx(), a, b, c, d, e, f); });
	nvg.set_function("reset_transform", [ctx]() { nvgResetTransform(ctx()); });
	nvg.set_function("scissor", [ctx](f32 x, f32 y, f32 w, f32 h) { nvgScissor(ctx(), x, y, w, h); });
	nvg.set_function("intersect_scissor",
	                 [ctx](f32 x, f32 y, f32 w, f32 h) { nvgIntersectScissor(ctx(), x, y, w, h); });
	nvg.set_function("reset_scissor", [ctx]() { nvgResetScissor(ctx()); });
	nvg.set_function("font_size", [ctx](f32 size) { nvgFontSize(ctx(), size); });
	nvg.set_function("font_face", [ctx](const char* name) { nvgFontFace(ctx(), name); });
	nvg.set_function("font_face_id", [ctx](i32 id) { nvgFontFaceId(ctx(), id); });
	nvg.set_function("font_blur", [ctx](f32 blur) { nvgFontBlur(ctx(), blur); });
	nvg.set_function("text_letter_spacing", [ctx](f32 spacing) { nvgTextLetterSpacing(ctx(), spacing); });
	nvg.set_function("text_line_height", [ctx](f32 height) { nvgTextLineHeight(ctx(), height); });
	nvg.set_function("text_align", [ctx](i32 align) { nvgTextAlign(ctx(), align); });
	nvg.set_function("text", [ctx](f32 x, f32 y, const char* text) { nvgText(ctx(), x, y, text, nullptr); });
	nvg.set_function("text_box",
	                 [ctx](f32 x, f32 y, f32 breakRowWidth, const char* text)
	                 { nvgTextBox(ctx(), x, y, breakRowWidth, text, nullptr); });
	nvg.set_function("text_bounds",
	                 [ctx](f32 x, f32 y, const char* text)
	                 {
		                 float bounds[4] = {};
		                 nvgTextBounds(ctx(), x, y, text, nullptr, bounds);
		                 return std::make_tuple(bounds[2] - bounds[0], bounds[3] - bounds[1]);
	                 });
	nvg.set_function("text_box_bounds",
	                 [ctx](f32 x, f32 y, f32 breakRowWidth, const char* text)
	                 {
		                 float bounds[4] = {};
		                 nvgTextBoxBounds(ctx(), x, y, breakRowWidth, text, nullptr, bounds);
		                 return std::make_tuple(bounds[2] - bounds[0], bounds[3] - bounds[1]);
	                 });
	nvg.set_function("create_font",
	                 [ctx](const char* name, const char* path) { return nvgCreateFont(ctx(), name, path); });
	nvg.set_function("load_default_font",
	                 [ctx](const char* name)
	                 {
		                 return nvgCreateFontMem(
		                         ctx(), name, const_cast<unsigned char*>(&funnel_sans_ttf[0]), FUNNEL_SANS_TTF_SIZE, 0);
	                 });
	nvg.set_function("find_font", [ctx](const char* name) { return nvgFindFont(ctx(), name); });
	nvg.set_function("create_image", [ctx](const char* path, i32 flags) { return nvgCreateImage(ctx(), path, flags); });
	nvg.set_function("image_size",
	                 [ctx](i32 image)
	                 {
		                 int w = 0;
		                 int h = 0;
		                 nvgImageSize(ctx(), image, &w, &h);
		                 return std::make_tuple(w, h);
	                 });
	nvg.set_function("delete_image", [ctx](i32 image) { nvgDeleteImage(ctx(), image); });

#define AXM_NVG_ENUM(table, prefix, name) (table)[#name] = CAST(prefix##name, i32)

	sol::table nvgWinding                                = state.create_table();
	nvg["winding"]                                       = nvgWinding;
	AXM_NVG_ENUM(nvgWinding, NVG_, CCW);
	AXM_NVG_ENUM(nvgWinding, NVG_, CW);

	sol::table nvgSolidity = state.create_table();
	nvg["solidity"]        = nvgSolidity;
	AXM_NVG_ENUM(nvgSolidity, NVG_, SOLID);
	AXM_NVG_ENUM(nvgSolidity, NVG_, HOLE);

	sol::table nvgLineCap = state.create_table();
	nvg["cap"]            = nvgLineCap;
	nvg["join"]           = nvgLineCap;
	AXM_NVG_ENUM(nvgLineCap, NVG_, BUTT);
	AXM_NVG_ENUM(nvgLineCap, NVG_, ROUND);
	AXM_NVG_ENUM(nvgLineCap, NVG_, SQUARE);
	AXM_NVG_ENUM(nvgLineCap, NVG_, BEVEL);
	AXM_NVG_ENUM(nvgLineCap, NVG_, MITER);

	sol::table nvgAlign = state.create_table();
	nvg["align"]        = nvgAlign;
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, LEFT);
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, CENTER);
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, RIGHT);
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, TOP);
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, MIDDLE);
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, BOTTOM);
	AXM_NVG_ENUM(nvgAlign, NVG_ALIGN_, BASELINE);

	sol::table nvgBlend = state.create_table();
	nvg["blend"]        = nvgBlend;
	AXM_NVG_ENUM(nvgBlend, NVG_, ZERO);
	AXM_NVG_ENUM(nvgBlend, NVG_, ONE);
	AXM_NVG_ENUM(nvgBlend, NVG_, SRC_COLOR);
	AXM_NVG_ENUM(nvgBlend, NVG_, ONE_MINUS_SRC_COLOR);
	AXM_NVG_ENUM(nvgBlend, NVG_, DST_COLOR);
	AXM_NVG_ENUM(nvgBlend, NVG_, ONE_MINUS_DST_COLOR);
	AXM_NVG_ENUM(nvgBlend, NVG_, SRC_ALPHA);
	AXM_NVG_ENUM(nvgBlend, NVG_, ONE_MINUS_SRC_ALPHA);
	AXM_NVG_ENUM(nvgBlend, NVG_, DST_ALPHA);
	AXM_NVG_ENUM(nvgBlend, NVG_, ONE_MINUS_DST_ALPHA);
	AXM_NVG_ENUM(nvgBlend, NVG_, SRC_ALPHA_SATURATE);

	sol::table nvgComposite = state.create_table();
	nvg["composite"]        = nvgComposite;
	AXM_NVG_ENUM(nvgComposite, NVG_, SOURCE_OVER);
	AXM_NVG_ENUM(nvgComposite, NVG_, SOURCE_IN);
	AXM_NVG_ENUM(nvgComposite, NVG_, SOURCE_OUT);
	AXM_NVG_ENUM(nvgComposite, NVG_, ATOP);
	AXM_NVG_ENUM(nvgComposite, NVG_, DESTINATION_OVER);
	AXM_NVG_ENUM(nvgComposite, NVG_, DESTINATION_IN);
	AXM_NVG_ENUM(nvgComposite, NVG_, DESTINATION_OUT);
	AXM_NVG_ENUM(nvgComposite, NVG_, DESTINATION_ATOP);
	AXM_NVG_ENUM(nvgComposite, NVG_, LIGHTER);
	AXM_NVG_ENUM(nvgComposite, NVG_, COPY);
	AXM_NVG_ENUM(nvgComposite, NVG_, XOR);

	sol::table nvgImageFlags = state.create_table();
	nvg["image_flags"]       = nvgImageFlags;
	AXM_NVG_ENUM(nvgImageFlags, NVG_IMAGE_, GENERATE_MIPMAPS);
	AXM_NVG_ENUM(nvgImageFlags, NVG_IMAGE_, REPEATX);
	AXM_NVG_ENUM(nvgImageFlags, NVG_IMAGE_, REPEATY);
	AXM_NVG_ENUM(nvgImageFlags, NVG_IMAGE_, FLIPY);
	AXM_NVG_ENUM(nvgImageFlags, NVG_IMAGE_, PREMULTIPLIED);
	AXM_NVG_ENUM(nvgImageFlags, NVG_IMAGE_, NEAREST);

	sol::table nvgTexture = state.create_table();
	nvg["texture"]        = nvgTexture;
	AXM_NVG_ENUM(nvgTexture, NVG_TEXTURE_, ALPHA);
	AXM_NVG_ENUM(nvgTexture, NVG_TEXTURE_, RGBA);

#undef AXM_NVG_ENUM

	sol::table im3d = state.create_table();
	axiom["im3d"]   = im3d;
	im3d.set_function("color",
	                  [](f32 r, f32 g, f32 b, sol::optional<f32> a) { return Im3d::Color(r, g, b, a.value_or(1.0f)); });
	sol::table im3dColors         = state.create_table();
	im3d["colors"]                = im3dColors;
	im3dColors["black"]           = Im3d::Color_Black;
	im3dColors["white"]           = Im3d::Color_White;
	im3dColors["gray"]            = Im3d::Color_Gray;
	im3dColors["red"]             = Im3d::Color_Red;
	im3dColors["green"]           = Im3d::Color_Green;
	im3dColors["blue"]            = Im3d::Color_Blue;
	im3dColors["magenta"]         = Im3d::Color_Magenta;
	im3dColors["yellow"]          = Im3d::Color_Yellow;
	im3dColors["cyan"]            = Im3d::Color_Cyan;
	im3dColors["pink"]            = Im3d::Color_Pink;
	im3dColors["orange"]          = Im3d::Color_Orange;
	im3dColors["gold"]            = Im3d::Color_Gold;
	im3dColors["brown"]           = Im3d::Color_Brown;
	im3dColors["purple"]          = Im3d::Color_Purple;
	im3dColors["teal"]            = Im3d::Color_Teal;
	im3dColors["navy"]            = Im3d::Color_Navy;

	sol::table im3dTextFlags      = state.create_table();
	im3d["text_flags"]            = im3dTextFlags;
	im3dTextFlags["align_left"]   = CAST(Im3d::TextFlags_AlignLeft, i32);
	im3dTextFlags["align_right"]  = CAST(Im3d::TextFlags_AlignRight, i32);
	im3dTextFlags["align_top"]    = CAST(Im3d::TextFlags_AlignTop, i32);
	im3dTextFlags["align_bottom"] = CAST(Im3d::TextFlags_AlignBottom, i32);
	im3dTextFlags["default"]      = CAST(Im3d::TextFlags_Default, i32);

	im3d.set_function("begin",
	                  [](sol::table       viewProj,
	                     u32              width,
	                     u32              height,
	                     const aml::Vec3& origin,
	                     const aml::Vec3& direction,
	                     f32              deltaTime)
	                  {
		                  if (g_Context.m_Im3dState == nullptr)
		                  {
			                  g_Context.m_Im3dState     = new vku::VkuIm3dState(vku::LoadIm3D(*g_Context.m_VK));
			                  g_Context.m_Im3dViewState = new vku::VkuIm3dViewState(
			                          vku::AddIm3dForViewport(*g_Context.m_VK,
			                                                  *g_Context.m_Im3dState,
			                                                  g_Context.m_VK->m_SwapchainImageRenderPass,
			                                                  false));
		                  }

		                  g_Context.m_Im3dViewProj = TableToMat4(viewProj);
		                  g_Context.m_Im3dWidth    = width;
		                  g_Context.m_Im3dHeight   = height;

		                  Im3d::AppData& appData   = Im3d::GetAppData();
		                  appData.m_viewportSize   = Im3d::Vec2(CAST(width, f32), CAST(height, f32));
		                  appData.m_viewOrigin     = ToIm3d(origin);
		                  appData.m_viewDirection  = ToIm3d(direction);
		                  appData.m_worldUp        = Im3d::Vec3(0.0f, 1.0f, 0.0f);
		                  appData.m_deltaTime      = deltaTime;
		                  appData.m_projScaleY     = 1.0f;
		                  Im3d::NewFrame();

		                  g_Context.m_Im3dActive = true;
	                  });
	im3d.set_function("set_color", [](Im3d::Color& color) { Im3d::SetColor(color); });
	im3d.set_function("set_alpha", [](f32 alpha) { Im3d::SetAlpha(alpha); });
	im3d.set_function("set_size", [](f32 size) { Im3d::SetSize(size); });
	im3d.set_function("push_color", [](Im3d::Color& color) { Im3d::PushColor(color); });
	im3d.set_function("pop_color", []() { Im3d::PopColor(); });
	im3d.set_function("push_matrix", []() { Im3d::PushMatrix(); });
	im3d.set_function("pop_matrix", []() { Im3d::PopMatrix(); });
	im3d.set_function("set_identity", []() { Im3d::SetIdentity(); });
	im3d.set_function("translate", [](const aml::Vec3& translation) { Im3d::Translate(ToIm3d(translation)); });
	im3d.set_function("rotate", [](const aml::Vec3& axis, f32 angle) { Im3d::Rotate(ToIm3d(axis), angle); });
	im3d.set_function("scale", [](f32 x, f32 y, f32 z) { Im3d::Scale(x, y, z); });
	im3d.set_function("begin_points", []() { Im3d::BeginPoints(); });
	im3d.set_function("begin_lines", []() { Im3d::BeginLines(); });
	im3d.set_function("begin_line_strip", []() { Im3d::BeginLineStrip(); });
	im3d.set_function("begin_line_loop", []() { Im3d::BeginLineLoop(); });
	im3d.set_function("begin_triangles", []() { Im3d::BeginTriangles(); });
	im3d.set_function("vertex", [](const aml::Vec3& position) { Im3d::Vertex(ToIm3d(position)); });
	im3d.set_function("end_primitive", []() { Im3d::End(); });
	im3d.set_function("point",
	                  [](const aml::Vec3& position, f32 size, Im3d::Color& color)
	                  { Im3d::DrawPoint(ToIm3d(position), size, color); });
	im3d.set_function("line",
	                  [](const aml::Vec3& a, const aml::Vec3& b, f32 size, Im3d::Color& color)
	                  { Im3d::DrawLine(ToIm3d(a), ToIm3d(b), size, color); });
	im3d.set_function("quad",
	                  [](const aml::Vec3& a, const aml::Vec3& b, const aml::Vec3& c, const aml::Vec3& d)
	                  { Im3d::DrawQuad(ToIm3d(a), ToIm3d(b), ToIm3d(c), ToIm3d(d)); });
	im3d.set_function("quad_filled",
	                  [](const aml::Vec3& a, const aml::Vec3& b, const aml::Vec3& c, const aml::Vec3& d)
	                  { Im3d::DrawQuadFilled(ToIm3d(a), ToIm3d(b), ToIm3d(c), ToIm3d(d)); });
	im3d.set_function("circle",
	                  [](const aml::Vec3& origin, const aml::Vec3& normal, f32 radius)
	                  { Im3d::DrawCircle(ToIm3d(origin), ToIm3d(normal), radius); });
	im3d.set_function("circle_filled",
	                  [](const aml::Vec3& origin, const aml::Vec3& normal, f32 radius)
	                  { Im3d::DrawCircleFilled(ToIm3d(origin), ToIm3d(normal), radius); });
	im3d.set_function("sphere", [](const aml::Vec3& origin, f32 radius) { Im3d::DrawSphere(ToIm3d(origin), radius); });
	im3d.set_function("sphere_filled",
	                  [](const aml::Vec3& origin, f32 radius) { Im3d::DrawSphereFilled(ToIm3d(origin), radius); });
	im3d.set_function("aligned_box",
	                  [](const aml::Vec3& min, const aml::Vec3& max)
	                  { Im3d::DrawAlignedBox(ToIm3d(min), ToIm3d(max)); });
	im3d.set_function("aligned_box_filled",
	                  [](const aml::Vec3& min, const aml::Vec3& max)
	                  { Im3d::DrawAlignedBoxFilled(ToIm3d(min), ToIm3d(max)); });
	im3d.set_function(
	        "arrow", [](const aml::Vec3& start, const aml::Vec3& end) { Im3d::DrawArrow(ToIm3d(start), ToIm3d(end)); });
	im3d.set_function("cylinder",
	                  [](const aml::Vec3& start, const aml::Vec3& end, f32 radius)
	                  { Im3d::DrawCylinder(ToIm3d(start), ToIm3d(end), radius); });
	im3d.set_function("capsule",
	                  [](const aml::Vec3& start, const aml::Vec3& end, f32 radius)
	                  { Im3d::DrawCapsule(ToIm3d(start), ToIm3d(end), radius); });
	im3d.set_function("axes", []() { Im3d::DrawXyzAxes(); });
	im3d.set_function(
	        "text",
	        sol::overload(
	                [](const aml::Vec3& position, const char* text)
	                { Im3d::Text(ToIm3d(position), 14.0f, Im3d::GetColor(), Im3d::TextFlags_Default, "%s", text); },
	                [](const aml::Vec3& position, const char* text, f32 size, Im3d::Color& color, i32 flags)
	                { Im3d::Text(ToIm3d(position), size, color, CAST(flags, Im3d::U32), "%s", text); }));

	auto createFlyCam = []() { return FlyCamController(g_Context.m_Engine->m_Input, g_Context.m_Engine->m_Window); };
	state.set_function("create_flycam_controller", createFlyCam);
	axiom.set_function("create_flycam_controller", createFlyCam);

	state.set_function("log", [](const char* message) { AXM_LOG("{}", message); });
}

int axm::lua::RunScript(AxiomEngine& engine, const char* scriptPath)
{
	sol::state                     state = CreateLuaState(engine);

	sol::protected_function_result load  = state.safe_script_file(scriptPath, sol::script_pass_on_error);
	if (!load.valid())
	{
		sol::error error = load;
		AXM_LOG_ERROR("Failed to run lua script '{}' : {}", scriptPath, error.what());
		return 1;
	}

	auto fetch = [&state](const char* name) -> sol::protected_function
	{
		sol::object global = state[name];
		if (global.is<sol::function>())
		{
			return global.as<sol::function>();
		}

		sol::object axiomObject = state["axiom"];
		if (axiomObject.is<sol::table>())
		{
			sol::object candidate = axiomObject.as<sol::table>()[name];
			if (candidate.is<sol::function>())
			{
				return candidate.as<sol::function>();
			}
		}

		return sol::protected_function();
	};

	sol::protected_function init   = fetch("init");
	sol::protected_function update = fetch("update");
	sol::protected_function render = fetch("render");
	sol::protected_function quit   = fetch("quit");

	if (!init.valid() && !update.valid() && !render.valid() && !quit.valid())
	{
		AXM_LOG_ERROR("Lua script '{}' does not define init/update/render/quit.", scriptPath);
	}

	auto safeCall = [](const sol::protected_function& function, const char* label, auto&&... args)
	{
		if (!function.valid())
		{
			return;
		}

		sol::protected_function_result result = function(args...);
		if (!result.valid())
		{
			sol::error error = result;
			AXM_LOG_ERROR("Lua {}() error : {}", label, error.what());
		}
	};

	const Timer initTimer = {};
	safeCall(init, "init");
	AXM_LOG("Lua init took {} ms.", initTimer.ElapsedMillisecondsF());
	AXM_LOG("Starting Axiom Main Loop");

	while (engine.m_Running)
	{
		if (engine.PreFrame())
		{
			g_Context.m_Viewport = viewports::GetFullscreenViewport(engine.m_Window.m_Window);

			safeCall(update, "update", CAST(engine.m_DeltaTime, f32));

			vku::commands::RecordGraphicsCommands(engine.m_GPU->m_VK,
			                                      [&](VkCommandBuffer& cmd, u32 frame)
			                                      {
				                                      g_Context.m_Cmd   = cmd;
				                                      g_Context.m_Frame = frame;

				                                      vku::render_passes::BeginSwapchainRenderPass(engine.m_GPU->m_VK,
				                                                                                   cmd);

				                                      safeCall(render, "render");

				                                      if (g_Context.m_Im3dActive && g_Context.m_Im3dState != nullptr &&
				                                          g_Context.m_Im3dViewState != nullptr)
				                                      {
					                                      Im3d::EndFrame();
					                                      vku::DrawIm3d(engine.m_GPU->m_VK,
					                                                    cmd,
					                                                    frame,
					                                                    *g_Context.m_Im3dState,
					                                                    *g_Context.m_Im3dViewState,
					                                                    g_Context.m_Im3dViewProj,
					                                                    g_Context.m_Im3dWidth,
					                                                    g_Context.m_Im3dHeight,
					                                                    true);
					                                      g_Context.m_Im3dActive = false;
				                                      }

				                                      nvgEndFrame(engine.m_GPU->m_VK.m_NanoVG);
				                                      vkCmdEndRenderPass(cmd);
			                                      });
		}
		else if (g_Context.m_Im3dActive)
		{
			Im3d::EndFrame();
			g_Context.m_Im3dActive = false;
		}

		profiler::ProfilerImGuiWindow(engine);
		engine.PostFrame();
	}

	safeCall(quit, "quit");

	if (g_Context.m_Im3dViewState != nullptr)
	{
		vku::FreeIm3dViewport(engine.m_GPU->m_VK, *g_Context.m_Im3dViewState);
		delete g_Context.m_Im3dViewState;
		g_Context.m_Im3dViewState = nullptr;
	}
	if (g_Context.m_Im3dState != nullptr)
	{
		vku::FreeIm3d(engine.m_GPU->m_VK, *g_Context.m_Im3dState);
		delete g_Context.m_Im3dState;
		g_Context.m_Im3dState = nullptr;
	}

	return 0;
}
