local transform = Transform()
transform.scale = vec3(0.2, 0.2, 0.2)

local camera = Camera()
local controller = create_flycam_controller()

local snap = 2.0
local shader = nil
local pipeline = nil
local drawables = {}

local function draw_scene_overlay()
    local width = axiom.window.width()
    local height = axiom.window.height()
    local nvg = axiom.nvg

    nvg.begin_path()
    nvg.rounded_rect(width - 260, 20, 240, 118, 8.0)
    nvg.fill_color(nvg.rgba(12, 14, 22, 200))
    nvg.fill()

    nvg.begin_path()
    nvg.rounded_rect(width - 260, 20, 240, 118, 8.0)
    nvg.stroke_color(nvg.rgba(90, 190, 255, 255))
    nvg.stroke_width(2.0)
    nvg.stroke()

    nvg.font_face("sans")
    nvg.font_size(20.0)
    nvg.text_align(nvg.align.LEFT + nvg.align.TOP)
    nvg.fill_color(nvg.rgba(255, 255, 255, 255))
    nvg.text(width - 242, 36, string.format("fps        %.1f", axiom.imgui.framerate()))
    nvg.text(width - 242, 64, string.format("meshes     %d", #drawables))
    nvg.text(width - 242, 92, string.format("psx snap   %.1f", snap))
end

local function draw_debug(viewProj, width, height, dt)
    axiom.im3d.begin(viewProj, width, height, camera.transform.position, camera.transform.forward, dt)
    axiom.im3d.axes()
    axiom.im3d.line(vec3(0, 0, 0), vec3(0, 3, 0), 3.0, axiom.im3d.colors.green)
    axiom.im3d.sphere(vec3(0, 1, 4), 0.6)
end

function init()
    axiom.nvg.load_default_font("sans")

    local vertDesc = axiom.graphics.pos_normal_uv_vertex_description()
    shader = axiom.graphics.create_shader("resources/shaders/psx.vert", "resources/shaders/psx.frag")
    pipeline = axiom.graphics.create_raster_pipeline(shader, vertDesc, axiom.graphics.default_raster_state())

    local model = axiom.assets.load_model("resources/models/sponza/Sponza.gltf")
    log(string.format("Loaded model with %d meshes", model:mesh_count()))

    for index = 1, model:mesh_count() do
        local material = axiom.graphics.create_material(shader)
        material:create_buffer(0, 0)

        local texture = model:diffuse(index)
        if texture:valid() then
            material:set_sampler("diffuse", texture)
        end

        table.insert(drawables, {
            mesh = model:mesh(index),
            material = material,
            uniforms = material:uniforms(0, 0)
        })
    end
end

function update(dt)
    local width = axiom.window.width()
    local height = axiom.window.height()
    camera.viewport_dimensions = vec2(width, height)
    controller:update(camera, dt)

    local mvp = axiom.math.mvp(transform, camera)
    for _, drawable in ipairs(drawables) do
        drawable.uniforms["modelViewProj"] = mvp
        drawable.uniforms["resolution"] = vec2(width, height)
        drawable.uniforms["snapIntensity"] = snap
    end

    axiom.imgui.begin("PSX Scene")
    snap = axiom.imgui.slider_float("Snap Intensity", snap, 0.0, 32.0)
    axiom.imgui.transform_edit(transform)
    axiom.imgui.camera_edit(camera)
    axiom.imgui.flycam_edit(controller)
    axiom.imgui.text(string.format("frame %.2f ms", 1000.0 * axiom.imgui.delta_time()))
    axiom.imgui.finish()

    draw_debug(axiom.math.view_projection(camera), width, height, dt)
end

function render()
    axiom.graphics.set_pipeline(pipeline)

    for _, drawable in ipairs(drawables) do
        axiom.graphics.draw(drawable.mesh, drawable.material)
    end

    draw_scene_overlay()
end

function quit()
    for _, drawable in ipairs(drawables) do
        drawable.material:free()
    end
    drawables = {}

    if pipeline then
        pipeline:free()
    end

    if shader then
        shader:free()
    end
end
