# RULES — MRF24J40 IoT System

## 📋 Reglas de Desarrollo

### 1. Ramas (Git Branching)

| Rama | Propósito | Base |
|------|-----------|------|
| `main` | Estable, releases oficiales | — |
| `release/Vx` | Desarrollo activo (V = versión) | main |
| `feature/*` | Features experimentales (opcional) | release/Vx |

**Regla:** Siempre trabajar en `release/Vx`. Merge a `main` solo cuando esté probado en la Raspberry Pi.

### 2. Compilación Remota

```bash
# Flujo completo
make compile-remote    # git push → ssh → git pull → make -j4

# O manualmente:
git add -A && git commit -m "mensaje" && git push origin release/V4
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && git pull && make -j4"
```

### 3. Estilo de Código

- **C++17** con `-Wall -Wextra`
- `snake_case` para funciones y variables
- `PascalCase` para clases y structs
- Prefijo `m_` para miembros privados
- Sufijo `_t` para clases
- `#ifndef` guards con `#define` en headers
- Doxygen `/** @brief */` en funciones públicas
- `std::unique_ptr` para ownership exclusivo
- No copiar clases con recursos (delete copy constructor)
- `const` correctness: parámetros de solo lectura como `const&`

### 4. Validación de Mensajes (v2.0.3)

Todo mensaje recibido debe pasar por `validateMessage()`:

1. **TTL > 0** — Si TTL=0, descartar
2. **Hash SHA-256** — Verificar integridad (cubre dest_mac + size + iv + ciphertext)
3. **Destino** — Si es para nosotros, descifrar. Si podemos reenviar, forwarding.
4. **Rol** — EndDevice no reenvía. Router/Coordinator/Mesh sí.

### 5. Formato de Trama

```
[dest_mac(8)][src_mac(8)][ttl(1)][size(2)][iv(16)][ciphertext(N)][hash(32)]
```

- Overhead: 67 bytes
- Hash: SHA-256 sobre (dest_mac + size + iv + ciphertext)
- Broadcast: dest_mac = 0xFFFFFFFFFFFFFFFF

### 6. Roles de Red

| Rol | ¿Reenvía? | ¿Flood? |
|-----|-----------|---------|
| EndDevice | ❌ | ❌ |
| Router | ✅ (si hay ruta) | ❌ |
| Coordinator | ✅ | ✅ (a todos los routers) |
| Mesh | ✅ | ❌ |

### 7. Configuración JSON

```json
{
  "pan_id": 51966,
  "channel": 20,
  "role": "end_device",
  "routing_table": [
    {"dest": "0000000000000002", "next": "0000000000000001"}
  ]
}
```

### 8. Menú Interactivo

| Opción | Función |
|--------|---------|
| 1 | Configurar modo MRF24J40 |
| 2 | Enviar mensaje cifrado |
| 3 | Escuchar mensajes |
| 4 | Generar QR |
| 5 | Probar OLED |
| 6 | Probar TFT |
| 7 | Ver/Editar configuración |
| 8 | Ver logs |
| **9** | **Configurar rol ZigBee** (nuevo v2.0.3) |
| **10** | **Tabla de enrutamiento** (nuevo v2.0.3) |
| **11** | **Estadísticas de validación** (nuevo v2.0.3) |
| 0 | Salir |

### 9. Librerías Permitidas

- BCM2835 (GPIO/SPI)
- OpenSSL EVP API (AES-256-CBC, SHA-256)
- libqrencode (QR)
- libpng (PNG)
- nlohmann/json (configuración JSON)
- zlib (compresión PNG)
- Mosquitto (MQTT, opcional)

### 10. Testing

- Compilar siempre en la Raspberry Pi antes de considerar un cambio completo
- Verificar con `make -j4` (sin warnings con `-Wall -Wextra`)
- Probar binario con `--version` y `--help`
- Verificar validación de mensajes con `--config` (muestra rol y estadísticas)
