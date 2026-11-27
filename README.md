> [!IMPORTANT]
> Be warned that this is **entirely** AI generated. While I am working to confirm its functionality, there may be bugs I am unable to fix on my own.
# Talonflame

Brewtendo/Pretendo network patches for Wii U consoles running **ISFShax**, plus an Aroma app to switch networks and accounts.

| Component | What it is | Goes on the SD card at |
|---|---|---|
| `wafel_inkay` | A stroopwafel plugin (`.ipx`). It applies Inkay's IOSU-side patches (network URLs and the SSL certificate-check patch) once at boot, before IOSU starts. | `sd:/wiiu/ios_plugins/wafel_inkay.ipx` |
| `switcher` | **Talonflame Switcher**, an Aroma app (`.wuhb`). It picks the boot network and swaps each user's `account.dat`. | `sd:/wiiu/apps/talonflame/talonflame.wuhb` |
| `companion` | **Talonflame Companion**, an Aroma module (`.wms`). It applies Inkay's patches to the Wii U's own software: DNS, eShop, Miiverse, account settings, and game matchmaking and peer-to-peer. | `sd:/wiiu/environments/aroma/modules/talonflame_companion.wms` |

### Replacing Inkay or Bnkay

The plugin and the companion together replace Inkay or Bnkay, so remove those before installing Talonflame:

- `sd:/wiiu/environments/aroma/plugins/Bnkay-brewtendo.wps` or `Inkay-pretendo.wps`
- `sd:/wiiu/environments/aroma/modules/Inkay-pretendo.wms`

Bnkay's module is also named `Inkay-pretendo.wms`. If either one stays installed, it reapplies its own network after boot, and the switcher's choice won't fully take effect.

The companion has no settings of its own. At startup it reads back the hostname the plugin patched into IOSU memory, then applies the matching network's patches. If it finds no patched hostname, it applies nothing (Nintendo Network). This means the two halves always agree for the current boot.

The URLs come from [BrewtendoNetwork/Bnkay](https://github.com/BrewtendoNetwork/Bnkay) (Brewtendo) and [PretendoNetwork/Inkay](https://github.com/PretendoNetwork/Inkay) (Pretendo). TVii is intentionally not handled; a separate app patches it.

## Requirements

- A console on firmware 5.5.x with ISFShax and stroopwafel (`00core.ipx` in `sd:/wiiu/ios_plugins/`).
- Aroma, to run the switcher app. The switcher needs Mocha, either from Aroma's MochaPayload or the `wafel_mocha` plugin, to reach system storage.

## Network profiles

| Profile | URL patches | SSL certificate-check patch |
|---|---|---|
| Nintendo Network | none | applied |
| Brewtendo Network | Brewtendo URLs | applied |
| Pretendo Network | Pretendo URLs | applied |

The profile is stored in a small config block inside `wafel_inkay.ipx` (see `wafel_inkay/source/tlnf_config.h`). The switcher rewrites that block in place on the SD card. Patches are applied at boot, so **a change only takes effect after powering the console off and back on**.

## Standby mode

After you turn the console off, it enters standby mode (yellow power light) a few minutes later. In standby, only IOSU runs, so it can handle SpotPass and background eShop downloads. Aroma never starts in standby, so Aroma-based patchers like Inkay and Bnkay can't patch it.

`wafel_inkay` patches IOSU every time it boots, standby included. So in standby, the console gets:

- the SSL certificate-check patch (confirmed working on hardware)
- the eShop backend URLs: ECS, NUS, IAS, CAS, tagaya and PLS
- the SpotPass URLs: task sheets, the policy-list host, npns, pushmo and npvk

The companion only patches apps that run on the main Wii U processor, so it isn't needed in standby.

The shipped default profile is **Brewtendo**.

## How the plugin differs from Inkay

Inkay runs on the PPC side after boot and pokes IOSU memory through Mocha. `wafel_inkay` runs inside IOSU, called by stroopwafel before the MMU is enabled and before any IOSU module starts. It writes the same values straight into the loaded fw.img segments, translating Inkay's virtual addresses with the fw.img program headers. Because of that:

- Inkay's two `.bss` patches can't be applied at boot. `.bss` is empty until the module runs.
  - The nppl policy-list URL is built from the patched `nppl` hostname instead.
  - The account-API URL format string is replaced at its source by a search over fw.img for `https://%s%saccount.nintendo.net/v%u/api/`.
- No "policy list refresh" poke is needed, because nothing has been fetched yet.
- On a firmware other than 5.5.x, the plugin applies nothing.

### SSL patch site

The SSL patch is one 32-bit write (`mov r0, #1`). Its address depends on the firmware:

- 5.5.5 and later: `0xE1019F78`
- 5.5.0–5.5.4: `0xE1019E84`

IOSU can't easily read the system version this early in boot, so the switcher detects it with `MCP_GetSystemVersion()` and stores the right mode in the config block. A fresh `.ipx` assumes 5.5.5 or later. On 5.5.0–5.5.4, open the switcher once and choose **Apply**.

## Switcher: accounts

Each user can keep one saved `account.dat` per network. Everything lives on the SD card:

```
sd:/wiiu/talonflame/accounts/<persistentId>/nintendo.dat
sd:/wiiu/talonflame/accounts/<persistentId>/brewtendo.dat
sd:/wiiu/talonflame/accounts/<persistentId>/pretendo.dat
sd:/wiiu/talonflame/accounts/<persistentId>/current.txt   network of the live account.dat
sd:/wiiu/talonflame/backups/<persistentId>-<time>.dat      safety copy before every swap
```

- **First run:** the app asks which network each linked user's current account belongs to. Users without a linked account are tagged automatically.
- **Apply network:** for every user, the app saves the live `account.dat` into the slot for its current network. Then:
  - If a saved account exists for the target network, the app restores it.
  - Otherwise it leaves the current account in place and warns you.

  Finally it writes the new profile into `wafel_inkay.ipx`.
- **Manage accounts:** for each user, you can mark which network the live account is for, save it to any slot, or restore any slot.

Every write to system storage is preceded by a safety copy to the SD card, then read back and compared. If the write fails, the original is put back.

To link a user to Brewtendo for the first time: switch the network to Brewtendo and power cycle. Then link the account in the system's account settings, using a user that isn't already linked to a Nintendo Network ID.

## Building

GitHub Actions builds both components on every push (`.github/workflows/build.yml`). Pushing a `v*` tag also creates a draft release with an SD-card-ready zip.

To build locally with Docker:

```bash
git clone https://github.com/StroopwafelCFW/stroopwafel.git
```

```bash
docker build wafel_inkay -t wafel-builder && docker run --rm -v "$PWD:/project" -w /project/wafel_inkay wafel-builder bash -c "STROOPWAFEL_ROOT=/project/stroopwafel make"
```

```bash
docker build switcher -t switcher-builder && docker run --rm -v "$PWD:/project" -w /project/switcher switcher-builder make
```

```bash
docker build companion -t companion-builder && docker run --rm -v "$PWD:/project" -w /project/companion companion-builder make
```

## Not yet tested on hardware

- The account-API string search assumes the original string exists verbatim in fw.img. The boot log line `replaced N occurrence(s)` shows whether it was found; check it on a serial or debug console.
- Whether IOS-ACT ever writes its in-memory copy back over a swapped `account.dat` before the power cycle. If a swap doesn't stick, restore the slot again and power off right away.
- The companion installs its function patches when the module starts. Bnkay installed them later, when its settings plugin loaded.

## License

GPLv3, the same as Inkay and Bnkay. See `LICENSE`.
