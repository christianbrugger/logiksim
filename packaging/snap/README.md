# Snap Packaging



## Create new Snap

```
snapcraft --verbosity debug --debug
```









## Dependencies

### Operating System

Ubuntu 24.04

### Snapcraft

```
sudo snap install snapcraft --classic
```

### LXD

```
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



