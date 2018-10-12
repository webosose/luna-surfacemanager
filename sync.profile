%modules = ( # path to module name map
    "WebOSCoreCompositor" => "$basedir/modules/weboscompositor",
    "WebOSCoreCompositorExtension" => "$basedir/modules/weboscompositorextension",
);

%moduleheaders = ( # restrict the module headers to those found in relative path
);

%classnames = (
);

%deprecatedheaders = (
);

# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#   - an empty string to use the same branch under test (dependencies will become "refs/heads/master" if we are in the master branch)
#
%dependencies = (
    "qtbase" => "",
    "qtdeclarative" => "",
    "qtwayland" => "",
);
%inject_headers = (
    "$basedir/modules/weboscompositor" => [
        "^qwayland-server-webos-surface-group.h",
        "^qwayland-server-webos-input-manager.h",
        "^qwayland-server-webos-foreign.h",
        "^wayland-webos-input-manager-server-protocol.h",
        "^wayland-webos-surface-group-server-protocol.h",
        "^wayland-webos-foreign-server-protocol.h",
    ],
);
@private_headers = ( qr/^qwayland-.*\.h/, qr/^wayland-.*-protocol\.h/ );
