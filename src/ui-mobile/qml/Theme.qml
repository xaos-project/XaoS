pragma Singleton
import QtQuick

/*
 * Theme — the single source of truth for XaoS Mobile's visual language.
 *
 * Every colour, radius, spacing step and type size used by the QML UI lives
 * here. Screens must not hardcode hex values: before this existed the tokens
 * were private properties of main.qml's root item, so the Community screens
 * could not reach them and grew a second, conflicting palette of their own.
 */
QtObject {
    id: theme

    // ---- Surfaces -------------------------------------------------------

    readonly property color bgDark:     "#0a0e1a"   // app ground
    readonly property color bgCard:     "#111827"   // cards, list rows
    readonly property color bgSurface:  "#1a2035"   // inputs, wells
    readonly property color bgElevated: "#141b2e"   // popup ground, above bgDark

    // ---- Accents --------------------------------------------------------

    readonly property color accentCyan:    "#00d2ff"  // primary action
    readonly property color accentMagenta: "#e94560"  // Julia, Community section
    readonly property color accentPurple:  "#6c63ff"
    readonly property color accentGreen:   "#00d282"  // success, share
    readonly property color accentAmber:   "#ffb432"
    readonly property color danger:        "#ff6b6b"  // errors

    // ---- Text -----------------------------------------------------------

    readonly property color textPrimary:   "#e8eaf0"
    readonly property color textSecondary: "#8892a4"
    readonly property color textDim:       "#4a5568"
    readonly property color textOnAccent:  "#08101f"

    // ---- Lines and scrim ------------------------------------------------

    readonly property color borderSubtle: "#1e293b"
    readonly property color borderBright: "#2a3a50"
    readonly property color scrim:        Qt.rgba(0, 0, 0, 0.60)

    // ---- Geometry -------------------------------------------------------

    readonly property int radiusSm: 8
    readonly property int radiusMd: 11
    readonly property int radiusLg: 13
    readonly property int radiusXl: 20

    readonly property int s1: 4
    readonly property int s2: 8
    readonly property int s3: 12
    readonly property int s4: 16
    readonly property int s5: 20
    readonly property int s6: 24

    // ---- Type -----------------------------------------------------------

    // fontEyebrow is the bold, letterspaced micro-label ("COLOR PALETTE",
    // "FRAC", "INVITE CODE") that carries most of the UI's character.
    readonly property int fontEyebrow: 9
    readonly property int fontXs:      10
    readonly property int fontSm:      11
    readonly property int fontBody:    13
    readonly property int fontMd:      14
    readonly property int fontLg:      18
    readonly property int fontXl:      20
    readonly property int fontDisplay: 24

    readonly property real trackingWide:  3
    readonly property real trackingTight: 2

    // ---- Motion ---------------------------------------------------------

    readonly property int durFast: 110
    readonly property int durBase: 160
    readonly property int durSlow: 200

    // ---- Layout ---------------------------------------------------------

    // Above this width the UI switches from phone to tablet/desktop layout.
    readonly property int wideBreakpoint: 700

    // Wide windows would otherwise stretch content edge to edge.
    readonly property int maxContentWidth:  1120
    readonly property int maxNavWidth:      760
    readonly property int maxDialogWidth:   420
    readonly property int maxPanelWidth:    400

    // Minimum comfortable touch target on a phone.
    readonly property int touchTarget: 44

    // ---- Icon font ------------------------------------------------------

    property FontLoader materialFontLoader: FontLoader {
        source: "qrc:/fonts/MaterialIcons-Regular.ttf"
    }
    readonly property string iconFont: materialFontLoader.name

    // ---- Helpers --------------------------------------------------------

    // Tint a token without restating its channels. Replaces the hand-written
    // Qt.rgba(0, 0.82, 1, 0.10) literals that were really copies of accentCyan.
    function alpha(c, a) {
        return Qt.rgba(c.r, c.g, c.b, a)
    }
}
