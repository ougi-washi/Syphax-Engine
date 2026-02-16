---
title: Glossary Terms
summary: Definitions for common docs vocabulary.
prerequisites:
  - None
---

# Glossary Terms

## context

The runtime owner of engine resources. Create with `se_context_create` and destroy last.

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
