Icon font for the UI library
############################

Made using Inkscape's builtin SVG font editor. The font editor is basically an
ad-hoc utility working on top of regular layers, and it might behave seemingly
broken if not understood:

-   A layer group matching the font name contains individual glyphs, each
    sublayer then matches a particular glyph.
-   Renaming the font or the glyph may result in the layer getting lost, so if
    something feels off, make sure the layer names match the glyphs.
-   Double-clicking or pressing "Edit" on a particular glyph in the list causes
    all other layers except the one matching it to get hidden. Any changes made
    afterwards are not applied to the actual glyph until you select the edited
    path and press "Get curves".
-   Only the actual path gets used for the glyph, so any lines with thick
    outlines will not produce anything. Make a copy of the layer and use
    "Stroke to path" to convert them to something the font can understand.
-   Similarly, only a single path is actually considered, thus you have to
    "Union" them first.

The output SVG contains the source layers and then a font element with
individual glyph paths, which are produced by the "Get curves" button. If a
change to a certain icon doesn't seem to be applied, check that the SVG diff
actually contains a change in the font as well, not just in the source layer.

Final conversion requires a FontForge install. Call `./ui-icon-font.pe` to
produce `src/Magnum/Ui/icons.ttf`.
