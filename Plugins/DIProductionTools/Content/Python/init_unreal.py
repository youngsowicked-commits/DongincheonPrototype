# Unreal executes this from the plugin's Content/Python directory when enabled.
# Only the Editor loads this code; never executed in packaged gameplay.
import unreal
try:
    import di_toolkit
    di_toolkit.register_menus()
except Exception as exc:
    unreal.log_error("[DI Tools] startup failed: {}".format(exc))
