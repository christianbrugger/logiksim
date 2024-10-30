# Snap Packaging



## Create new Snap

Prepare the repository:

```
git clone https://github.com/christianbrugger/logiksim/
cd logiksim

python external/checkout.py
```

Make symlink, so `snapcraft.yaml` is found

```
ln -s packaging/snap snap
```

Build new snap (run at repo root)

```bash
snapcraft --verbosity debug --debug
```

Test install it

```bash
sudo snap install ./logiksim_*_amd64.snap --dangerous --devmode
```

Run the new snap

```
logiksim
logiksim.test-core
logiksim.test-gui -d y
```

Adjust settings:

```bash
nvim ~/snap/logiksim/current/.config/LogikSim/LogikSim/2.2.0/gui_settings.json
```









## Dependencies

### Operating System

Ubuntu 24.04

### Snapcraft

```bash
sudo snap install snapcraft --classic
```

### LXD

```bash
sudo snap install lxd
```

```bash
sudo lxd init --minimal
```

```bash
sudo usermod -a -G lxd $USER
```

Re-login user or restart.

```bash
sudo ufw allow in on lxdbr0
sudo ufw route allow in on lxdbr0
sudo ufw route allow out on lxdbr0
```



