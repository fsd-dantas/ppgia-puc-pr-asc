# Setup do Ambiente — Registro de Procedimentos

Documento de apoio. Registra os passos executados para montar o ambiente do experimento.
Não faz parte do relatório entregável, mas serve como referência e rastreabilidade.

---

## 1. Host — cisei-optiplex-mon-01

**Hardware:** Dell OptiPlex 9010, i7-3770, 16 GB RAM, 3× HDD 465.8 GB  
**OS:** Ubuntu 24.04.4 LTS

### 1.1 Instalação do KVM

```bash
sudo apt install -y qemu-kvm libvirt-daemon-system virtinst virt-manager bridge-utils
sudo usermod -aG kvm,libvirt ciseiadmin
kvm-ok   # → KVM acceleration can be used
```

### 1.2 Disco dedicado para a VM

`/dev/sdb2` selecionado como disco raw da VM (sem filesystem, 465.8 GB).  
Disco separado do OS host (`sda`) para isolar I/O nos experimentos.

### 1.3 Download da ISO do guest

```bash
wget -P /var/lib/libvirt/images \
  https://releases.ubuntu.com/24.04/ubuntu-24.04.2-live-server-amd64.iso
```

---

## 2. Guest — criação da VM

### 2.1 Comando virt-install

```bash
sudo virt-install \
  --name guest-ubuntu2404 \
  --ram 4096 \
  --vcpus 4 \
  --disk /dev/sdb2,bus=virtio,format=raw \
  --osinfo ubuntu24.04 \
  --network network=default,model=virtio \
  --graphics none \
  --console pty,target_type=serial \
  --location /var/lib/libvirt/images/ubuntu-24.04.2-live-server-amd64.iso,kernel=casper/vmlinuz,initrd=casper/initrd \
  --extra-args 'console=ttyS0,115200n8 serial'
```

### 2.2 Pós-instalação

```bash
# Verificar VM ativa
sudo virsh list --all

# Conectar ao console
sudo virsh console guest-ubuntu2404

# Instalar ferramentas de monitoramento no guest
sudo apt install -y sysstat iotop strace python3
```

---

## 3. Identificação do processo da VM no host

```bash
# Encontrar PID do processo QEMU da VM
ps -eo pid,ppid,comm,%cpu,%mem --sort=-%cpu | grep qemu

# Acompanhar durante experimentos
pidstat -p <PID> 1
```

---

## 4. Pendências

- [ ] Concluir instalação da VM após download da ISO
- [ ] Anotar PID do processo qemu-system-x86_64 no host
- [ ] Instalar sysstat, iotop, strace no guest
- [ ] Executar Experimento 1 (CPU)
- [ ] Executar Experimento 2 (escrita disco) — obrigatório
- [ ] Executar Experimento 3 (leitura disco)
- [ ] Executar Experimento 4 (syscalls)
- [ ] Preencher tabela consolidada no relatorio.md
- [ ] Redigir análise e conclusão
