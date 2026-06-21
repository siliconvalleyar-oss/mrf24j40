#!/bin/bash
set -e

echo "Generando web moderna..."

mkdir -p web

# ---------- Generar index.html ----------
cat > index.html <<'EOF'
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>MRF24J40 · Proyecto</title>
    <link rel="stylesheet" href="web/style.css" />
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:ital,opsz,wght@0,14..32,100..900;1,14..32,100..900&display=swap" rel="stylesheet">
    <script src="https://cdn.jsdelivr.net/npm/marked/marked.min.js"></script>
</head>
<body>
    <div id="app">
        <!-- SIDEBAR -->
        <aside id="sidebar">
            <div class="sidebar-header">
                <h2>📡 <span>MRF24J40</span></h2>
                <button id="toggleSidebar" title="Colapsar">◀</button>
            </div>
            <div class="sidebar-search">
                <input type="text" id="fileSearch" placeholder="🔍 Buscar archivo..." />
            </div>
            <div id="tree-container">
                <ul id="tree-root"></ul>
            </div>
            <div class="sidebar-footer">
                <button id="btn-readme">📖 README</button>
                <button id="btn-todo">📋 TODO</button>
                <button id="btn-version">🏷️ Versión</button>
            </div>
        </aside>

        <!-- CONTENIDO PRINCIPAL -->
        <main id="main-content">
            <div id="content-area">
                <div id="welcome">
                    <div class="logo-badge">⚡</div>
                    <h1>MRF24J40 · IoT System</h1>
                    <p class="subtitle">Plataforma de comunicación ZigBee con cifrado, displays y MQTT</p>
                    <div class="status-cards">
                        <div class="status-card"><span>📦</span> v2.0.2</div>
                        <div class="status-card"><span>🌿</span> release/V3</div>
                        <div class="status-card"><span>📁</span> 234 archivos</div>
                    </div>
                    <hr />
                    <p class="hint">Selecciona un archivo en el panel izquierdo para explorar su contenido.</p>
                </div>
            </div>
        </main>
    </div>

    <script src="web/script.js"></script>
    <script>
        // ------------------------------------------------------------
        // ESTRUCTURA DE ARCHIVOS (inyectada desde Bash)
        // ------------------------------------------------------------
        const fileTree = [];

        // ------------------------------------------------------------
        // FUNCIONES DEL ÁRBOL
        // ------------------------------------------------------------
        function buildTree(files) {
            const root = {};
            files.forEach(file => {
                const parts = file.path.split('/');
                let current = root;
                parts.forEach((part, idx) => {
                    if (idx === parts.length - 1) {
                        if (!current._files) current._files = [];
                        current._files.push({ name: part, path: file.path });
                    } else {
                        if (!current[part]) current[part] = {};
                        current = current[part];
                    }
                });
            });
            return root;
        }

        function renderTree(node, path = '') {
            let html = '<ul>';
            const keys = Object.keys(node).filter(k => k !== '_files');
            keys.sort((a, b) => a.localeCompare(b));
            for (const key of keys) {
                const subPath = path ? path + '/' + key : key;
                html += `<li><span class="folder" data-path="${subPath}">📁 ${key}</span>`;
                html += renderTree(node[key], subPath);
                html += '</li>';
            }
            if (node._files) {
                node._files.sort((a, b) => a.name.localeCompare(b.name));
                for (const file of node._files) {
                    const ext = file.name.split('.').pop().toLowerCase();
                    const icon = ext === 'md' ? '📄' :
                                 ext === 'cpp' || ext === 'hpp' || ext === 'h' ? '📜' :
                                 ext === 'json' ? '📋' :
                                 ext === 'sh' ? '⚙️' : '📎';
                    html += `<li><span class="file" data-path="${file.path}">${icon} ${file.name}</span></li>`;
                }
            }
            html += '</ul>';
            return html;
        }

        // Renderizar árbol inicial
        const treeData = buildTree(fileTree);
        const treeRoot = document.getElementById('tree-root');
        treeRoot.innerHTML = renderTree(treeData);

        // ------------------------------------------------------------
        // FILTRO DE BÚSQUEDA (sidebar)
        // ------------------------------------------------------------
        document.getElementById('fileSearch').addEventListener('input', function(e) {
            const query = e.target.value.toLowerCase().trim();
            const items = treeRoot.querySelectorAll('.file, .folder');
            items.forEach(el => {
                const text = el.textContent.toLowerCase();
                const parentLi = el.closest('li');
                if (parentLi) {
                    if (text.includes(query) || query === '') {
                        parentLi.style.display = '';
                    } else {
                        parentLi.style.display = 'none';
                    }
                }
            });
        });

        // ------------------------------------------------------------
        // NAVEGACIÓN POR CLIC
        // ------------------------------------------------------------
        document.addEventListener('click', function(e) {
            const target = e.target.closest('.file, .folder');
            if (!target) return;

            const path = target.dataset.path;
            if (target.classList.contains('file')) {
                loadFile(path);
            } else if (target.classList.contains('folder')) {
                const ul = target.nextElementSibling;
                if (ul && ul.tagName === 'UL') {
                    ul.style.display = ul.style.display === 'none' ? '' : 'none';
                }
            }
        });

        // ------------------------------------------------------------
        // CARGA DE ARCHIVOS
        // ------------------------------------------------------------
        async function loadFile(path) {
            const area = document.getElementById('content-area');
            try {
                const response = await fetch(path);
                if (!response.ok) throw new Error(`HTTP ${response.status}`);
                let content = await response.text();

                const ext = path.split('.').pop().toLowerCase();
                const isMarkdown = ['md', 'markdown'].includes(ext);

                if (isMarkdown) {
                    area.innerHTML = `<div class="markdown-body">${marked.parse(content)}</div>`;
                } else {
                    const lang = ext === 'cpp' || ext === 'hpp' || ext === 'h' ? 'cpp' :
                                 ext === 'json' ? 'json' :
                                 ext === 'sh' ? 'bash' : 'text';
                    area.innerHTML = `<pre><code class="language-${lang}">${escapeHtml(content)}</code></pre>`;
                }
                // Scroll al inicio
                document.getElementById('main-content').scrollTop = 0;
            } catch (err) {
                area.innerHTML = `<div class="error-box">❌ Error: ${err.message}</div>`;
            }
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        // ------------------------------------------------------------
        // BOTONES RÁPIDOS
        // ------------------------------------------------------------
        document.getElementById('btn-readme').addEventListener('click', () => loadFile('README.md'));
        document.getElementById('btn-todo').addEventListener('click', () => loadFile('TODO.md'));
        document.getElementById('btn-version').addEventListener('click', () => loadFile('VERSION.txt'));

        // ------------------------------------------------------------
        // TOGGLE SIDEBAR (responsive)
        // ------------------------------------------------------------
        document.getElementById('toggleSidebar').addEventListener('click', function() {
            const sidebar = document.getElementById('sidebar');
            const main = document.getElementById('main-content');
            const isCollapsed = sidebar.classList.toggle('collapsed');
            main.classList.toggle('expanded');
            this.textContent = isCollapsed ? '▶' : '◀';
        });

        // Cargar README por defecto
        loadFile('README.md');
    </script>
</body>
</html>
EOF

# ---------- Generar style.css ----------
cat > web/style.css <<'EOF'
/* ============================================================
   FUENTES Y RESET
   ============================================================ */
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    background: #f8fafc;
    color: #1e293b;
    height: 100vh;
    overflow: hidden;
}

/* ============================================================
   LAYOUT PRINCIPAL
   ============================================================ */
#app {
    display: flex;
    height: 100vh;
}

/* ============================================================
   SIDEBAR
   ============================================================ */
#sidebar {
    width: 320px;
    min-width: 320px;
    background: #ffffff;
    border-right: 1px solid #e2e8f0;
    display: flex;
    flex-direction: column;
    transition: width 0.3s ease, min-width 0.3s ease, padding 0.3s ease;
    padding: 1.25rem 1rem;
    box-shadow: 2px 0 12px rgba(0,0,0,0.04);
    z-index: 10;
}

#sidebar.collapsed {
    width: 0;
    min-width: 0;
    padding: 0 0;
    overflow: hidden;
    border-right: none;
}

.sidebar-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 1.25rem;
}
.sidebar-header h2 {
    font-size: 1.15rem;
    font-weight: 700;
    color: #0f172a;
    letter-spacing: -0.02em;
}
.sidebar-header h2 span {
    background: linear-gradient(135deg, #2563eb, #7c3aed);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
}
#toggleSidebar {
    background: #f1f5f9;
    border: none;
    border-radius: 8px;
    width: 32px;
    height: 32px;
    font-size: 1rem;
    cursor: pointer;
    color: #475569;
    transition: background 0.15s;
}
#toggleSidebar:hover {
    background: #e2e8f0;
}

/* --- Búsqueda --- */
.sidebar-search {
    margin-bottom: 1rem;
}
.sidebar-search input {
    width: 100%;
    padding: 0.6rem 0.8rem;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    font-size: 0.9rem;
    background: #f8fafc;
    transition: border 0.15s, box-shadow 0.15s;
    outline: none;
    color: #1e293b;
}
.sidebar-search input:focus {
    border-color: #2563eb;
    box-shadow: 0 0 0 3px rgba(37, 99, 235, 0.15);
}

/* --- Árbol de archivos --- */
#tree-container {
    flex: 1;
    overflow-y: auto;
    padding-right: 4px;
    margin-right: -4px;
}
#tree-container::-webkit-scrollbar {
    width: 6px;
}
#tree-container::-webkit-scrollbar-track {
    background: transparent;
}
#tree-container::-webkit-scrollbar-thumb {
    background: #cbd5e1;
    border-radius: 10px;
}

#tree-root {
    list-style: none;
    font-size: 0.9rem;
}
#tree-root ul {
    list-style: none;
    padding-left: 1.2rem;
    border-left: 1px dashed #e2e8f0;
    margin-left: 0.3rem;
}
#tree-root li {
    margin: 2px 0;
}
#tree-root .file,
#tree-root .folder {
    display: block;
    padding: 0.3rem 0.6rem;
    border-radius: 6px;
    cursor: pointer;
    transition: background 0.1s, color 0.1s;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}
#tree-root .file:hover,
#tree-root .folder:hover {
    background: #f1f5f9;
}
#tree-root .folder {
    font-weight: 600;
    color: #0f172a;
}
#tree-root .file {
    color: #475569;
    font-weight: 400;
}
#tree-root .file[data-path$=".md"] {
    color: #7c3aed;
}
#tree-root .file[data-path$=".cpp"],
#tree-root .file[data-path$=".hpp"],
#tree-root .file[data-path$=".h"] {
    color: #0d9488;
}
#tree-root .file[data-path$=".json"] {
    color: #b45309;
}
#tree-root .file[data-path$=".sh"] {
    color: #2563eb;
}

/* --- Footer sidebar --- */
.sidebar-footer {
    margin-top: 1rem;
    display: flex;
    gap: 0.5rem;
    flex-wrap: wrap;
    padding-top: 1rem;
    border-top: 1px solid #e2e8f0;
}
.sidebar-footer button {
    flex: 1;
    min-width: 60px;
    padding: 0.5rem 0.6rem;
    border: none;
    border-radius: 10px;
    font-size: 0.8rem;
    font-weight: 500;
    cursor: pointer;
    background: #f1f5f9;
    color: #334155;
    transition: background 0.15s, transform 0.1s;
}
.sidebar-footer button:hover {
    background: #e2e8f0;
    transform: translateY(-1px);
}
.sidebar-footer button:active {
    transform: translateY(0);
}

/* ============================================================
   CONTENIDO PRINCIPAL
   ============================================================ */
#main-content {
    flex: 1;
    overflow-y: auto;
    padding: 2rem 2.5rem;
    background: #f8fafc;
    transition: margin-left 0.3s ease;
}

#main-content.expanded {
    margin-left: 0;
}

#content-area {
    max-width: 1000px;
    margin: 0 auto;
}

/* ============================================================
   WELCOME / INICIO
   ============================================================ */
#welcome {
    text-align: center;
    padding: 3rem 1rem 2rem;
}
.logo-badge {
    font-size: 3rem;
    margin-bottom: 0.5rem;
}
#welcome h1 {
    font-size: 2.8rem;
    font-weight: 700;
    background: linear-gradient(135deg, #0f172a, #2563eb);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    letter-spacing: -0.02em;
}
.subtitle {
    font-size: 1.15rem;
    color: #64748b;
    margin-top: 0.25rem;
}
.status-cards {
    display: flex;
    justify-content: center;
    gap: 1.2rem;
    margin: 1.5rem 0 2rem;
    flex-wrap: wrap;
}
.status-card {
    background: white;
    padding: 0.5rem 1.4rem;
    border-radius: 40px;
    font-size: 0.9rem;
    font-weight: 500;
    color: #1e293b;
    box-shadow: 0 2px 8px rgba(0,0,0,0.04);
    border: 1px solid #e2e8f0;
}
.status-card span {
    margin-right: 0.3rem;
}
.hint {
    color: #94a3b8;
    font-size: 0.95rem;
    margin-top: 0.5rem;
}
#welcome hr {
    margin: 1.8rem auto;
    border: 0;
    height: 1px;
    background: linear-gradient(to right, transparent, #e2e8f0, transparent);
    max-width: 400px;
}

/* ============================================================
   MARKDOWN RENDER
   ============================================================ */
.markdown-body {
    font-size: 1rem;
    line-height: 1.7;
    color: #1e293b;
}
.markdown-body h1,
.markdown-body h2,
.markdown-body h3 {
    color: #0f172a;
    margin-top: 1.8em;
    margin-bottom: 0.5em;
    font-weight: 600;
}
.markdown-body a {
    color: #2563eb;
    text-decoration: none;
}
.markdown-body a:hover {
    text-decoration: underline;
}
.markdown-body pre {
    background: #1e293b;
    border-radius: 12px;
    padding: 1.2rem;
    overflow-x: auto;
    color: #e2e8f0;
}
.markdown-body code {
    font-family: 'JetBrains Mono', 'Fira Code', monospace;
    font-size: 0.9rem;
    background: #f1f5f9;
    padding: 0.2rem 0.4rem;
    border-radius: 4px;
}
.markdown-body pre code {
    background: transparent;
    padding: 0;
}
.markdown-body ul,
.markdown-body ol {
    padding-left: 1.8rem;
}
.markdown-body blockquote {
    border-left: 4px solid #2563eb;
    padding-left: 1.2rem;
    color: #475569;
    margin: 1rem 0;
}
.markdown-body table {
    border-collapse: collapse;
    width: 100%;
    font-size: 0.9rem;
}
.markdown-body th,
.markdown-body td {
    border: 1px solid #e2e8f0;
    padding: 0.5rem 0.8rem;
    text-align: left;
}
.markdown-body th {
    background: #f1f5f9;
    font-weight: 600;
}

/* ============================================================
   CODE BLOCKS (raw)
   ============================================================ */
pre {
    background: #1e293b;
    border-radius: 12px;
    padding: 1.2rem;
    overflow-x: auto;
    font-size: 0.9rem;
    font-family: 'JetBrains Mono', monospace;
    color: #e2e8f0;
    box-shadow: 0 4px 12px rgba(0,0,0,0.05);
}
code {
    font-family: 'JetBrains Mono', monospace;
    background: #f1f5f9;
    padding: 0.2rem 0.4rem;
    border-radius: 4px;
}

/* ============================================================
   ERROR BOX
   ============================================================ */
.error-box {
    background: #fef2f2;
    color: #b91c1c;
    padding: 1.2rem;
    border-radius: 12px;
    border: 1px solid #fecaca;
    font-weight: 500;
    text-align: center;
}

/* ============================================================
   RESPONSIVE
   ============================================================ */
@media (max-width: 768px) {
    #sidebar {
        width: 0;
        min-width: 0;
        padding: 0;
        border-right: none;
    }
    #sidebar.collapsed {
        width: 0;
        min-width: 0;
    }
    #main-content {
        padding: 1rem;
        margin-left: 0;
    }
    #main-content.expanded {
        margin-left: 0;
    }
    #welcome h1 {
        font-size: 2rem;
    }
    .status-cards {
        gap: 0.6rem;
    }
    .status-card {
        font-size: 0.8rem;
        padding: 0.4rem 0.8rem;
    }
}
EOF

# ---------- Generar script.js ----------
cat > web/script.js <<'EOF'
// La lógica principal está en index.html
// Este archivo existe para mantener la estructura
console.log('✅ Web MRF24J40 cargada correctamente.');
EOF

# ---------- Generar el JSON de la estructura de archivos ----------
echo "Generando índice de archivos..."
STRUCTURE_JSON=$(find . -type f \( \
    -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.md' -o -name '*.txt' -o \
    -name '*.json' -o -name '*.dio' -o -name 'Makefile' -o -name '*.sh' -o -name '*.c' -o \
    -name '*.yml' -o -name '*.yaml' \
\) -not -path "./web/*" -not -path "./obj/*" -not -path "./bin/*" -not -path "./.git/*" \
   -not -path "./*/*.o" -not -path "./*/*.d" | sort | awk '
BEGIN { printf "["; first=1 }
{
    gsub(/\\/, "\\\\", $0); gsub(/"/, "\\\"", $0)
    path = $0
    n = split(path, parts, "/")
    name = parts[n]
    dir = ""
    for (i=1; i<n; i++) { if (i>1) dir=dir "/"; dir=dir parts[i] }
    if (substr(dir,1,1)==".") dir=substr(dir,2)
    if (dir=="") dir="."
    if (!first) printf ","
    first=0
    printf "{\"path\":\"%s\",\"name\":\"%s\",\"dir\":\"%s\"}", path, name, dir
}
END { print "]" }')

# Inyectar el JSON en el archivo index.html
sed -i "s|const fileTree = \[\];|const fileTree = $STRUCTURE_JSON;|" index.html

echo ""
echo "✅ ¡Web generada correctamente!"
echo "📂 Archivos creados:"
echo "   - index.html (raíz)"
echo "   - web/style.css"
echo "   - web/script.js"
echo ""
echo "🌐 Para servirla:"
echo "   python3 -m http.server 8080"
echo "   Abre http://localhost:8080/index.html"
