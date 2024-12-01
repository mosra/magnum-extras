Snippets that possibly generate output for Magnum documentation
###############################################################

### ui-*.svg

Created by Inkscape from `doc/artwork/ui-*.svg` by saving *a copy* as Optimized
SVG. On fresh installations you need the `scour` and `python-tinycss2` packages
for it.

-   enabling all possible options in the dialog, saving
-   cleaning up the `<svg>` header (removing `version`, `xmlns`) in an editor
-   converting `width` and `height` to a `style=""`, *keeping* `viewBox`
-   adding `class="m-image"`
-   removing all layers that have `display: none`
