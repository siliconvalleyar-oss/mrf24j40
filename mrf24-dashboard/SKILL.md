# 🧠 Skill: Herramientas de Documentación — MRF24J40 IoT Dashboard & C++

## Stack del Dashboard Web

### Frontend
| Herramienta | Uso | Versión/Detalle |
|-------------|-----|-----------------|
| **HTML5** | Estructura del dashboard | SPA vanilla (sin frameworks) |
| **CSS3** | Estilos, animaciones, responsive | Variables CSS (`:root`), Flexbox, Grid |
| **JavaScript** | Lógica del dashboard | Vanilla JS (sin frameworks) |
| **highlight.js** | Syntax highlighting de código | v11.9.0, tema `github-dark` |
| **marked.js** | Renderizado de Markdown a HTML | CDN latest |
| **Google Fonts** | Tipografía | `JetBrains Mono` (mono) + `Inter` (sans) |

### Funcionalidades implementadas
- Terminal header con dots (macOS-style) y badge de versión
- Sidebar colapsable con explorador de archivos
- Buscador en tiempo real de archivos
- Dashboard con cards de estadísticas
- Visor de código con syntax highlighting (highlight.js)
- Visor de Markdown con renderizado (marked.js)
- Gráfico de tráfico en tiempo real (Canvas API + requestAnimationFrame)
- Diagrama de arquitectura SVG con dots animados
- Barra de estado con LEDs parpadeantes
- Animaciones CSS (signal pulse, flow dots, fade-in)
- Responsive design (mobile adaptado)
- ResizeObserver para canvas responsive
- Iconos por extensión de archivo (📄, 📜, ⚡, etc.)

### SVG Animations
- `animate` element para radio/pulse de anillos de señal
- `animateMotion` para dots de datos volando entre nodos
- `@keyframes` CSS para blinking LEDs y flow dots

### Canvas (Real-time Chart)
- `requestAnimationFrame` para animación fluida (60fps)
- 3 líneas simultáneas: paquetes/s (sólida), RSSI (dash), LQI (dot)
- Área rellena bajo la línea de paquetes con gradiente
- Grid dinámica y etiquetas de eje Y
- Datos simulados con funciones sinusoidales + ruido
- `ResizeObserver` para redimensionamiento automático

### Diseño
- Tema oscuro terminal (#0a0e17 fondo)
- Sistema de diseño basado en variables CSS
- Gradientes (`linear-gradient`) para títulos
- Animaciones hover con `transform: translateY`
- Sombras con glow sutiles
- Scrollbar personalizada

---

## Proyecto C++ Embebido (v2.0.3)

### Estructura
```
mrf24j40/
├── mrf24_security/   → 🔐 Binario unificado (mrf24j40_iot)
│   ├── hal/          →   GPIO, SPI, I2C
│   ├── drivers/      →   MRF24J40, SSD1306, ST7789, QR
│   ├── services/     →   Crypto, FileSystem, Timer
│   └── application/  →   RadioManager, Menu
├── mrf24_tx/         → 🚀 Transmisor legacy
├── mrf24_rx/         → 📡 Receptor legacy
├── mrf24-dashboard/  → 🌐 Dashboard web
├── ARCHITECTURE.md   → 📐 Documentación de arquitectura
├── RULES.md          → 📋 Reglas de desarrollo
└── scripts_tools/    → 🔧 Scripts de configuración
```

### Tecnologías
| Tecnología | Uso |
|------------|-----|
| **C++17** | Lenguaje principal |
| **BCM2835** | GPIO, SPI, I2C (Raspberry Pi) |
| **OpenSSL 3.x** | AES-256-CBC + SHA-256 (EVP API) |
| **libqrencode** | Generación de códigos QR |
| **libpng** | Exportar QR a PNG |
| **nlohmann/json** | Configuración JSON persistente |
| **Mosquitto** | MQTT cliente/servidor (opcional) |
| **Make** | Sistema de compilación |

### Novedades v2.0.3
- ✅ **Roles ZigBee**: EndDevice, Router, Coordinator, Mesh
- ✅ **Validación SHA-256**: hash sobre (dest + size + iv + ciphertext)
- ✅ **Enrutamiento con TTL**: forwarding automático, broadcast
- ✅ **Menú extendido**: opciones 9-11 para rol, rutas, stats
- ✅ **config.json persistente**: rol y tabla de rutas en JSON

### Formato de Trama Segura
```
[dest_mac(8)][src_mac(8)][ttl(1)][size(2)][iv(16)][ciphertext(N)][hash(32)]
Overhead: 67 bytes  |  Hash: SHA-256(dest + size + iv + ciphertext)
```

### Compilación
```bash
make                         # Compilar todo (con -DENABLE_JSON)
make -C mrf24_security       # Solo proyecto unificado
make compile-remote          # Push + SSH + compilar en Pi
make serve-dashboard         # Servir dashboard web (localhost:8080)
make run-unificado           # Compilar y ejecutar
```

---

## Git Workflow

### Ramas y Flujo
```
release/V4  → Rama de desarrollo activo (v2.0.3)
main        → Rama estable
```

### Flujo diario
```bash
# 1. Editar localmente
# 2. Pushear a release/V4
git add -A && git commit -m "v2.0.3: descripción" && git push origin release/V4
# 3. Compilar en Pi (automático o manual)
make compile-remote
```

### Scripts útiles
| Script | Función |
|--------|---------|
| `scripts_tools/installBCM2835.sh` | Instalar librería BCM2835 |
| `scripts_tools/installMosquitto.sh` | Instalar Mosquitto MQTT |
| `scripts_tools/installLibs.sh` | Instalar todas las dependencias |
| `scripts_tools/config_radio.sh` | Configurar interfaz de radio |
| `scripts_tools/gpio.sh` | Configurar pines GPIO |
| `scripts_tools/build_single_tx_rx.sh` | Compilar tx o rx individual |
| `scripts_tools/git_commit.sh` | Automatizar commits |
| `scripts_tools/search_git_date.sh` | Buscar commits por fecha |

---

## Nuevas Herramientas v2.0.3

### Roles y Enrutamiento
```cpp
// Configurar rol
radioManager.setRole(NodeRole::Router);

// Agregar ruta (destino → siguiente salto)
radioManager.addRoute(0x0002, 0x0001);

// Buscar ruta
uint64_t nextHop;
if (radioManager.findRoute(0x0002, nextHop)) {
    // forward via nextHop
}

// Verificar si puede reenviar
if (radioManager.canForward()) { ... }
```

### Validación de Mensajes
```cpp
// buildSecureMessage construye trama con hash SHA-256
auto frame = radioManager.buildSecureMessage(dest_mac, iv, ciphertext);

// validateMessage verifica hash + TTL + destino
auto result = radioManager.validateMessage(buf, len);
if (result.valid && result.for_us) { /* descifrar */ }
if (result.should_forward) { /* reenviar */ }
```

### JSON (nlohmann)
```cpp
// Config.json incluye rol y routing_table
{
    "role": "router",
    "routing_table": [
        {"dest": "0000000000000002", "next": "0000000000000001"}
    ]
}
```

---

## Estilo de código C++

### Convenciones
- `snake_case` para funciones y variables (namespace)
- `PascalCase` para clases y structs
- `m_` prefijo para miembros privados
- `_t` sufijo para clases y tipos
- `#ifndef` guards para headers
- Doxygen-style `/** @brief */` documentación
- `std::unique_ptr` para ownership exclusivo
- RAII para gestión de recursos
- `const&` para parámetros de solo lectura
- `delete` copy constructor para clases con recursos

### Formato
```cpp
namespace application {

class MiClase_t {
public:
    explicit MiClase_t(uint8_t param);
    ~MiClase_t();

    MiClase_t(const MiClase_t&) = delete;
    MiClase_t& operator=(const MiClase_t&) = delete;

    void hacerAlgo();

private:
    uint8_t m_miembro;
    std::unique_ptr<services::Crypto_t> m_crypto;
};

} // namespace application
```

---

## Estilo Markdown para documentación

```markdown
# Título principal (H1)

## Sección (H2)

### Subsección (H3)

**Texto en negrita** o *cursiva*

`código inline`

\```cpp
// Bloque de código
\```

| Tabla | Columna 2 |
|-------|-----------|
| Dato  | Valor     |

- [x] Tarea completada
- [ ] Tarea pendiente
```

---

## Técnicas de Desarrollo

### 1. Protocolo Seguro con Hash

El hash SHA-256 se calcula sobre `[dest_mac] + [size] + [iv] + [ciphertext]`. Esto:
- **Vincula** el mensaje a un destinatario específico (evita replay a otro destino)
- **Autentica** el contenido (solo quien conoce la clave de cifrado puede generar el hash correcto)
- **Detecta** manipulación en tránsito

### 2. Enrutamiento con TTL

- TTL máximo = 10 saltos
- Cada router decrementa TTL al reenviar
- TTL=0 → descartar (evita bucles infinitos)
- Broadcast (0xFF...FF) reenviado por todos los routers

### 3. Separación de Capas

- **hal/** → abstracción de hardware (SPI, GPIO, I2C)
- **drivers/** → circuitos específicos (MRF24J40, SSD1306, ST7789)
- **services/** → lógica de negocio reutilizable (crypto, filesystem, timer)
- **application/** → orquestación y UI (RadioManager, Menu)

### 4. Configuración JSON Persistente

- Configuración guardada en `config.json` con nlohmann/json
- Rol y tabla de rutas persisten entre reinicios
- Compilación condicional con `-DENABLE_JSON`
- Fallback a valores por defecto si no hay JSON
