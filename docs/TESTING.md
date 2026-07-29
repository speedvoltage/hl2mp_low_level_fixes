# Runtime test plan

## Basic load test

1. Install the VDF and binary.
2. Restart the HL2DM server.
3. Run `meta list`.
4. Run `meta info <plugin-id>`.
5. Confirm the server log reports one resolved signature and the expected platform offset.
6. Change maps twice.
7. Confirm there are no repeated warnings, exceptions, or crashes.

## Dissolve trigger regression test

Use a test map containing:

```text
trigger_weapon_dissolve
one or more conduit entities matching its emitter name
an ordinary weapon that can enter the trigger
```

The crash condition requires the trigger to retain a weapon handle after the weapon entity has already been removed. Remove the collected weapon before the trigger's next dissolve think.

Expected behavior with the plugin:

```text
The server does not crash.
The invalid entry is removed.
The log reports the number of stale handles removed.
Other valid weapons continue through the stock dissolve behavior.
```

The plugin intentionally leaves the stock 0.5 to 1.5 second dissolve interval unchanged.

## Unload test

With no dissolve callback currently executing:

```text
meta unload <plugin-id>
```

Expected behavior:

```text
The detour is removed.
The plugin unloads cleanly.
The log reports the cumulative number of stale handles sanitized.
```

Restart the server before installing a replacement build.
