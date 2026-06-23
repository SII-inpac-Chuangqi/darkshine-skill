---
name: darkshine-container-workflow
description: Run DarkSHINE simulation (DSimu) & reconstruction (DAna) via container image. Includes limit setting with real Baseline1.6 cutflow.
version: 1.1.0
platforms: [linux]
metadata:
  hermes:
    tags: [DarkSHINE, container, simulation, reconstruction, limit, workflow]
---

# DarkSHINE — Container Workflow

Run DarkSHINE simulation (DSimu), reconstruction (DAna), and physics limit setting using the pre-built container image.

## Image

AlmaLinux 9 based image with full dependency stack:
- Geant4 10.6.3, ROOT 6.30.06 (Geom,TMVA,Eve,Gui,RGL,EG), ACTS (xuliang-v30), ONNX Runtime 1.19.2
- System deps: GSL, yaml-cpp, XercesC, Eigen3, HDF5, Boost, protobuf, xxhash

Dockerfile + Apptainer def at `/Users/jonah/SII/image/`.

## 1. Pull / Build

```bash
docker pull <registry>/darkshine-simulation:alma9
# or build locally
cd /Users/jonah/SII/image && docker build -t darkshine-simulation:alma9 .
```

## 2. Run container

```bash
docker run --rm -it -v $(pwd):/work darkshine-simulation:alma9
# or apptainer
apptainer run --bind $(pwd):/work darkshine-alma9.sif
```

## 3. Build darkshine-simulation source

```bash
git clone https://github.com/SII-inpac-Chuangqi/darkshine-simulation.git src
cd src && git lfs pull
mkdir build install && cd build
cmake ../src -DCMAKE_INSTALL_PREFIX=../install \
  -DWITH_GEANT4_UIVIS=ON -DBUILD_ACTS=ON -DBUILD_HDF5=ON \
  -DBUILD_DANA=ON -DBUILD_DSIMU=ON -DBUILD_DDIS=ON -DBUILD_TOOLS=ON \
  -DBUILD_ONNX=ON
make -j$(nproc) && make install
```

## 4. Run simulation (DSimu)

```bash
mkdir run && cd run
cp ../src/DP_simu/scripts/default.yaml .
cp ../src/DP_simu/scripts/magnet_1.5.root .
../install/bin/DSimu -y default.yaml -b 1000
# Output: dp_simu.root
```

### Key YAML config
```yaml
RootManager:
  outfile_Name: "dp_simu.root"
  Event_Start_Number: 1
  Event_Stop_Number: 1000
MagField:
  uniform_mag_field: false
  tag_Tracker_MagField: [0,"tesla",-1.5,"tesla",0,"tesla"]
Global:
  save_geometry: false
  signal_production: false
```

## 5. Run reconstruction (DAna)

```bash
cd run
cat > config.txt << 'EOF'
InputFile = dp_simu.root
InputGeoFile = dp_simu.root
OutputFile = dp_ana.root
AlgoManager.Verbose = 0
Algorithm.List = Digitizer MCTruthAnalysis Tracking RecECAL RecHCAL
Tracking.if_strip = 1
Tracking.if_smear = 1
Tracking.con_field = -1.5
Tracking.seed_method = 1
Tracking.find_method = 1
Tracking.Rec_fit_method = 2
Tracking.Tag_fit_method = 2
RecECAL.StaggeredECAL = 1
EOF
../install/bin/DAna -c config.txt
# Output: dp_ana.root
```

## 6. Limit Setting — End-to-End

The limit setting uses pre-computed `fullcutflow` histograms from reconstructed signal samples. The pipeline:

```
DSimu (signal) → DAna → sampleMaker/miniTree.C → getHist.cxx → fullcutflow
                                                                    ↓
                                                            runLimit.C
                                                          (90% CL ε²)
```

### 6.1 Signal sample production

10 dark photon mass points (Baseline 1.6): 1, 10, 20, 50, 100, 200, 500, 1000, 1500, 2000 MeV.
Generated with DSimu biasing (cross-section enhancement), reconstructed with DAna.

### 6.2 Cutflow histogram structure

The `fullcutflow` histogram in `ana_signal_XXXXMeV_fullout.root` has 8 bins:

| Bin | Label | Cut |
|-----|-------|-----|
| 1 | All | Total events |
| 2 | passDQ | Data quality |
| 3 | 1track | Exactly 1 tag + 1 rec track |
| 4 | MissingP | p_miss > 4 GeV |
| 5 | HCal | E_HCAL^total < 30 MeV |
| 6 | HCalCell10 | E_HCAL^max cell < 10 MeV |
| 7 | HCalCell0.1 | E_HCAL^max cell < 0.1 MeV |
| 8 | ECal | E_ECAL^total < 2.5 GeV |

### 6.3 Run limit setting

Working macro at `/Users/jonah/.hermes/skills/darkshine/darkshine-container-workflow/scripts/runLimit.C`.

```bash
root -l -q -b runLimit.C
```

Produces:
- `sigcutflow_1.6.png` — cumulative cutflow efficiency vs mA'
- `limit_1.6.png` — 90% CL ε² exclusion limit

### 6.4 Limit formula

90% CL Poisson upper limit (Asimov dataset, b only):
```
Ups = 0.5 × F⁻¹_χ²(0.95, 2(Nb+1)) − Nb
N_exp = Ups / ε_sig
ε² = N_exp / (σ_A' × 0.1X₀ × ρ_tgt × EOT × N_A/M_W × 10⁻³⁶)
```

Parameters:
- Nb = estimated background yield per 3×10¹⁴ EOT
- EOT = 3×10¹⁴ (configurable)
- σ_A' = dark photon cross section (pb/ε²) from CalcHEP
- Target: W, 0.1 X₀ (6.76 g/cm²), M_W = 184 g/mol

### 6.5 Input data

Signal `fullout` files expected at a configurable path. For real Baseline 1.6 data on IHEP cluster:
```
/lustre/collider/chenjing/DarkSHINE/Analysis/Baseline1p6/sampleMaker/outputFiles/
├── ana_signal_0001MeV_fullout.root
├── ana_signal_0010MeV_fullout.root
├── ...
└── ana_signal_2000MeV_fullout.root
```

Local mirror: `/cefs/higgs/zhuyifan/DarkSHINE/input/signal/`.

## 7. Plot customization

```cpp
TLegend *leg = new TLegend(0.12, 0.12, 0.88, 0.32);  // left-bottom, wide
leg->SetTextSize(0.025);   // small text
leg->SetBorderSize(0);     // no border
leg->SetNColumns(2);       // two columns
leg->SetMargin(0.05);      // tight column spacing
```

## Output files

| File | Producer | Content |
|------|----------|---------|
| `dp_simu.root` | DSimu | MC truth, tracker hits, calorimeter hits |
| `dp_ana.root` | DAna | Reconstructed tracks, cutflows |
| `sigcutflow_1.6.png` | runLimit.C | Cumulative efficiency vs mA' |
| `limit_1.6.png` | runLimit.C | 90% CL ε² exclusion limit |

## Pitfalls

- **LFS files**: After clone, run `git lfs pull` for magnetic field `.root` files
- **ROOT components**: Build with `-Dtmva=ON -Deve=ON -Dgui=ON -Dgdml=ON -Drgl=ON -Deg=ON` (Geom, TMVA, Eve, Gui, RGL, EG). Missing `xxhash-devel` causes cmake failure.
- **Container on lxlogin**: Use apptainer def file format, skip `dnf update` (fakeroot incompatibility), use AlmaLinux 9 (CRB works).
- **fullcutflow bin map**: bin 3=1track, bin 4=MissingP, bin 7=HCalCell<0.1MeV, bin 8=ECal<2.5GeV. Do NOT confuse with bin numbering in older baselines.
- **GFW downloads**: Use `codeload.github.com` for source tarballs behind GFW.
