# Snap Packaging



## Create new Snap

Make symlink, so `snapcraft.yaml` is found

```
ln -s packaging/snap snap
```

Build new snap (run at repo root)

```bash
(clean cache as of below) OR specific step:
snapcraft clean logiksim

snapcraft --verbosity debug --debug
```

Test install it

```bash
sudo snap remove logiksim && sudo snap install ./logiksim_*_amd64.snap --dangerous

sudo snap remove logiksim && sudo snap install ./logiksim_*_amd64.snap --dangerous --devmode
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



## Publish Snap

```
snapcraft login
```

Upload it as a new release candiate.

```
snapcraft push --release candidate logiksim_*_amd64.snap
```

Then publish it as release under 

https://snapcraft.io/logiksim/releases





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









# Clean Snap Cache

Storage used

 ```
 sudo lxc storage info default --bytes
 ```

More details storage info

```
sudo apt install zfsutils-linux
sudo zfs list
```



List instances:

```
lxc list --all-projects
```

Delete each `snapcraft-*` instance one by one, e.g:

```
lxc delete --project=snapcraft snapcraft-logiksim-on-amd64-for-amd64-
```



