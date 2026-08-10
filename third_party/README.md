# third_party (local only)

Large / redistributed blobs stay **out of git**. Create on this machine:

```bash
mkdir -p third_party/mpss4
# Place Intel MPSS 4.4.1 archive and extracted card images here, e.g.:
#   third_party/mpss4/mpss-4.4.1-linux.tar
#   third_party/mpss4/bzImage-knl-lb  (or paths inside extract)

git clone https://github.com/jjkeijser/mpss.git third_party/jjkeijser-mpss
```

Then:

```bash
bash scripts/phase1_mpss4_readiness.sh
# On Rocky 8 / kernel 4.18 only:
bash scripts/try_build_mpss4_modules.sh
```

See `docs/research/20260805-phi-7220p-phase1-mpss4.md`.
