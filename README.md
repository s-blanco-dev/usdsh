<p align="center">
  <img src="https://github.com/user-attachments/assets/c4291a32-a880-4f6b-91a3-66730f07375a" alt="usdSh" width="200">
</p>

---

Intérprete de comandos simple escrito en C con fines educativos. Incluye:
- ✔️ **Ejecución de comandos externos** (vía `fork()` y `execvp`)
- ✔️ **Built-ins básicos**: `cd`, `exit`, `help`
- ✔️ **Redirección de salida**:
  - Sobrescribir archivo: `comando > archivo.txt`
  - Agregar al final del archivo: `comando >> archivo.txt`
- ✔️ **Pipes**: Soporta hasta un (1) pipe `comando1 | comando2`, ignora todo a partir del segundo pipe, así que ojo.
- ✔️ **Manejo de señales y TTY**:
  - Control total de terminal por parte del proceso en primer plano
  - Ignora `SIGTTOU`, `SIGTTIN`, `SIGTSTP` en la shell
  - Restaura señales por defecto en procesos hijos
---
- Documentación disponible próximamente (en algún futuro lejano)
