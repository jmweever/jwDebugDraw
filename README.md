# jwDebugDraw

A plugin for [Godot Engine](https://github.com/godotengine/godot) that provides an API for drawing debug shapes, lines, and text in 3D world space and 2D screen space. Designed for zero-hassle debug visualization: no nodes, no instances, no setup.

![Screenshot of the plugin](/assets/screenshot-1.png?raw=true)

## Features

- **Works Everywhere:** It works both in-editor and in-game.
- **Immediate-Mode API:** Call from anywhere, no node required.
- **3D & 2D:** Draw in world space or directly on screen.
- **Rich Shape Library:** Lines, arcs, circles, boxes, spheres, capsules, cylinders, cones, arrows, axes, and more.
- **Text Rendering:** World-space and screen-space text, with optional outlines and backgrounds.
- **Physics Frame Support:** Safe to call from `_physics_process`.
- **Draw Flags:** Control fill, silhouette-only, background, and screen-space rendering per shape.
- **Cross-Platform:** Windows, Linux, macOS, Android, iOS, Web

![Screenshot of the plugin](/assets/screenshot-2.png?raw=true)

## Installation

1. Download the latest release zip from the [Releases](https://github.com/jmweever/jwDebugDraw/releases/latest) page and extract it.
2. Copy the `jwdebugdraw/` folder into your Godot project's `addons/` directory.
3. That's it! No autoload or scene setup needed.

## Usage

All methods are available directly on the `JWDebugDraw` class with no instance needed.

### GDScript

```gdscript
func _process(delta):
    JWDebugDraw.draw_line(Vector3(0, 0, 0), Vector3(1, 0, 0))
    JWDebugDraw.draw_sphere(Vector3(0, 1, 0), 0.5)
    JWDebugDraw.draw_text("Hello!", Vector3(0, 0, 1))
```

### C#
```csharp
// Coming in a future release...
```

> \[!NOTE]
> Shapes are drawn for the current frame only and are automatically cleared each frame, so you just call the draw methods every frame for persistent visuals.

## Draw Flags

Most draw methods accept an optional `flags` parameter to modify rendering behavior.

| Flag                   | Description                                    |
|------------------------|------------------------------------------------|
| `DRAW_NONE`            | Default, no special behavior                   |
| `DRAW_FILLED`          | Draw shape as filled/solid                     |
| `DRAW_BACKGROUND`      | Draw a translucent background behind the shape |
| `DRAW_SILHOUETTE_ONLY` | Draw only the outer silhouette edges           |
| `DRAW_SCREEN_SPACE`    | Draw in screen space, rather than world space  |

> [!TIP]
> Flags can also be combined: `DRAW_FILLED | DRAW_SCREEN_SPACE`

## API Reference

| Method                                                                           | Description                                                 |
|----------------------------------------------------------------------------------|-------------------------------------------------------------|
| `draw_text(text, position, text_color, outline_color, background_color, flags)`  | Draw text at a 3D world position                            |
| `draw_text_xform(text, transform, ...)`                                          | Draw text with a full 3D transform _(supports orientation)_ |
| `draw_text_2d(text, position, ...)`                                              | Draw text at a 2D screen position                           |
| `draw_point(position, size, color, flags)`                                       | Draw a point                                                |
| `draw_line(start, end, color, flags)`                                            | Draw a line between two points                              |
| `draw_ray(origin, direction, color, flags)`                                      | Draw a ray with an arrowhead                                |
| `draw_triangle(vertex_a, vertex_b, vertex_c, color, flags)`                      | Draw a triangle                                             |
| `draw_arc(position, axis, radius, angle_start, angle_end, color, flags)`         | Draw an arc                                                 |
| `draw_curve(points, tension, segments, color, flags)`                            | Draw a curved line through a set of points                  |
| `draw_circle(position, axis, radius, color, flags)`                              | Draw a circle                                               |
| `draw_circle_xform(transform, color, flags)`                                     | Draw a circle with a transform                              |
| `draw_sphere(position, radius, color, flags)`                                    | Draw a sphere                                               |
| `draw_sphere_xform(transform, color, flags)`                                     | Draw a sphere with a transform                              |
| `draw_sphere_arc(position, radius, angle_start, angle_end, color, flags)`        | Draw a partial sphere arc                                   |
| `draw_sphere_arc_xform(transform, angle_start, angle_end, color, flags)`         | Draw a sphere arc with a transform                          |
| `draw_hemisphere(position, radius, up, color, flags)`                            | Draw a hemisphere                                           |
| `draw_box(position, size, color, flags)`                                         | Draw an axis-aligned box                                    |
| `draw_box_xform(transform, color, flags)`                                        | Draw a box with a transform                                 |
| `draw_frustum(position, size_start, size_end, length, color, flags)`             | Draw a frustum _(tapered box)_                              |
| `draw_frustum_xform(transform, size_start, size_end, color, flags)`              | Draw a frustum with a transform                             |
| `draw_pyramid(position, size, height, color, flags)`                             | Draw a pyramid                                              |
| `draw_pyramid_xform(transform, color, flags)`                                    | Draw a pyramid with a transform                             |
| `draw_capsule(start, end, radius, color, flags)`                                 | Draw a capsule                                              |
| `draw_capsule_xform(transform, color, flags)`                                    | Draw a capsule with a transform                             |
| `draw_tapered_capsule(start, end, radius_start, radius_end, color, flags)`       | Draw a tapered capsule                                      |
| `draw_tapered_capsule_xform(transform, radius_start, radius_end, color, flags)`  | Draw a tapered capsule with a transform                     |
| `draw_cylinder(start, end, radius, color, flags)`                                | Draw a cylinder                                             |
| `draw_cylinder_xform(transform, color, flags)`                                   | Draw a cylinder with a transform                            |
| `draw_tapered_cylinder(start, end, radius_start, radius_end, color, flags)`      | Draw a tapered cylinder                                     |
| `draw_tapered_cylinder_xform(transform, radius_start, radius_end, color, flags)` | Draw a tapered cylinder with a transform                    |
| `draw_cylinder_arc(start, end, radius, angle_start, angle_end, color, flags)`    | Draw a partial cylinder arc                                 |
| `draw_cylinder_arc_xform(transform, angle_start, angle_end, color, flags)`       | Draw a cylinder arc with a transform                        |
| `draw_cone(position, radius, height, color, flags)`                              | Draw a cone                                                 |
| `draw_cone_xform(transform, color, flags)`                                       | Draw a cone with a transform                                |
| `draw_arrow(start, end, head_radius, head_length, color, flags)`                 | Draw an arrow                                               |
| `draw_axis(transform, size)`                                                     | Draw an XYZ axis gizmo _(red/green/blue)_                   |