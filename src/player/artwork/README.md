# Exporting the default.glb file

1.  Ensure you have the
    [Blender glTF Exporter](https://github.com/KhronosGroup/glTF-Blender-Exporter)
    installed and enabled.
2.  Open the `default.blend` file, make desired changes and save the blend
    file.
3.  Select the camera, press <kbd>T</kbd> to open the tools pane and from
    *Animation* select *Bake Action*
4.  Enable all checkboxes and press *Bake*. **Do not save the blend file after
    this point.**
5.  Delete the now-unneeded curve
6.  Export to glb:
    -   disable *Export selected only*
    -   disable *Export texture coordinates*, *tangents* and *colors*
    -   enable *Export cameras*, otherwise the camera animation won't play
    -   overwrite the `default.glb` file
7.  Verify the animation plays back correctly
8.  Commit the updated `default.glb` file, do a sanity check that the size
    didn't change much. If commiting the `default.blend` file, make sure you
    don't commit the baked version.
