local camera = Camera()
local controller = create_flycam_controller()

local time = 0.0
local showGrid = true
local showShapes = true
local lineWidth = 2.0
local bounces = 0.0

local function draw_grid()
    local extent = 10
    local axis = axiom.im3d.colors.gray
    local minor = axiom.im3d.color(0.28, 0.32, 0.42, 1.0)

    for i = -extent, extent do
        local color = (i == 0) and axis or minor
        axiom.im3d.line(vec3(i, 0, -extent), vec3(i, 0, extent), 1.0, color)
        axiom.im3d.line(vec3(-extent, 0, i), vec3(extent, 0, i), 1.0, color)
    end
end

local function draw_shapes()
    local bounce = math.abs(math.sin(time)) * 3.0

    axiom.im3d.sphere(vec3(-3, 1 + bounce, 0), 0.75)

    axiom.im3d.set_color(axiom.im3d.color(0.4, 0.9, 1.0, 1.0))
    axiom.im3d.push_matrix()
    axiom.im3d.translate(vec3(3, 1, 0))
    axiom.im3d.rotate(vec3(0, 1, 0), time)
    axiom.im3d.arrow(vec3(0, 0, 0), vec3(2, 0, 0))
    axiom.im3d.pop_matrix()

    axiom.im3d.aligned_box(vec3(-1, 0.5, 3), vec3(1, 2.5, 5))
    axiom.im3d.cylinder(vec3(3, 0, 3), vec3(3, 2, 3), 0.5)
    axiom.im3d.capsule(vec3(5, 0.5, 3), vec3(5, 2, 3), 0.4)

    axiom.im3d.text(vec3(0, 2.4, 0), "world origin")
end

local function draw_debug()
    local width = axiom.window.width()
    local height = axiom.window.height()

    axiom.im3d.begin(axiom.math.view_projection(camera), width, height,
        camera.transform.position, camera.transform.forward, 0.016)

    axiom.im3d.set_size(lineWidth)
    axiom.im3d.axes()

    if showGrid then
        draw_grid()
    end

    if showShapes then
        draw_shapes()
    end
end

function update(dt)
    time = time + dt
    bounces = bounces + 1.0

    local width = axiom.window.width()
    local height = axiom.window.height()
    camera.viewport_dimensions = vec2(width, height)
    controller:update(camera, dt)

    axiom.imgui.begin("Im3d Debug Draw")
    local _, showGridValue = axiom.imgui.checkbox("Show Grid", showGrid)
    showGrid = showGridValue
    local _, showShapesValue = axiom.imgui.checkbox("Show Shapes", showShapes)
    showShapes = showShapesValue
    lineWidth = axiom.imgui.slider_float("Line Width", lineWidth, 1.0, 8.0)
    axiom.imgui.text(string.format("time %.2f", time))
    axiom.imgui.finish()

    draw_debug()
end

function render()
end

function quit()
end
