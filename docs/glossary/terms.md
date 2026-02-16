---
title: Glossary Terms
summary: Definitions for common docs vocabulary.
prerequisites:
  - None
---

# Glossary Terms

## context

The runtime owner of engine resources. Create with `se_context_create` and destroy last.

## fixed timestep

A constant simulation step (for example `1.0f / 60.0f`) used to keep physics/simulation behavior stable.

## world

A container that owns simulation state such as physics bodies or navigation occupancy.

## body

A simulated object in a world, usually static, kinematic, or dynamic.

## shape

Collision or query geometry attached to a body.

## trigger

A non-solid collision shape that reports overlap events without applying physical response.

## handle

An opaque identifier (`s_handle`) used to reference engine-managed objects.

## scene

A container for renderable objects and draw behavior (`se_scene_2d` or `se_scene_3d`).

## instance

One transform/material occurrence of an object in a scene.

## shader

A GPU program pair used for rendering.

## raycast

A query along a line segment to test visibility or collision.

## dt (delta time)

Elapsed time between frames.

## action map

A mapping from raw input sources to named runtime actions.

## resource scope

Path group defined by `SE_RESOURCE_INTERNAL`, `SE_RESOURCE_PUBLIC`, or `SE_RESOURCE_EXAMPLE`.

## action mapping

Mapping from physical input to named actions.

## deterministic step

Simulation update where identical inputs produce identical results.

## interpolation

Calculating intermediate values between keyframes or states.

## trace span

Named timing block measured by debug tools.
