# Git Bootstrap

`git` is not available in this terminal session, so run these commands in your own shell where git is installed.

## On Neil's Machine (this workspace)
```bash
cd "c:/Users/neilb/OneDrive/Desktop/neil barot/college/Sem-5 ~Spring 26'/Systems Programming/P2"
git init
git add .
git commit -m "Initialize P2 scaffold for Neil and Hriday"
git branch -M main
git checkout -b neil-dirwalk-main
```

## Create Remote and Push
```bash
git remote add origin <your-repo-url>
git push -u origin neil-dirwalk-main
git push -u origin main
```

## On Hriday's iLab Linux
```bash
git clone <your-repo-url>
cd P2
git checkout -b hriday-freq-sim origin/main
```

## Integration
```bash
# after both feature branches are ready
git checkout -b integration-final origin/main
git merge neil-dirwalk-main
git merge hriday-freq-sim
make
./p2compare <directory-with-txt-files>
```
