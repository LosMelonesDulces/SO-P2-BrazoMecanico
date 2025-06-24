#include <linux/module.h>       // Necesario para todos los módulos
#include <linux/kernel.h>       // Necesario para KERN_INFO
#include <linux/fs.h>           // Necesario para la estructura file_operations

// Todos estos son de la libreria de raspberry
#include <linux/init.h>       // Necesario para las macros __init y __exit
#include <linux/cdev.h>       // Necesario para el dispositivo de caracteres (cdev)
#include <linux/device.h>     // Necesario para class_create/device_create
#include <linux/uaccess.h>    // Necesario para copy_from_user
#include <asm/io.h>           // Necesario para ioremap/iounmap
#include <linux/delay.h>      // Necesario para msleep

// --- Metadatos del modulo ---
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jose Eduardo Cruz Vargas\nDario Ramses Gutierrez Rodriguez\nDennis Alejandro Jimenez Campos\nMarco Vinicio Rivera Serrano");
MODULE_DESCRIPTION("Driver para el brazo robotico RoboticTEC (control directo de GPIO)");
MODULE_VERSION("1.0");

// --- Constantes y variables globales ---
#define DEVICE_NAME "robotic_hand"
#define CLASS_NAME "robotictec"

static dev_t major_number;
static struct class* robotic_class = NULL;
static struct device* robotic_device = NULL;
static struct cdev my_cdev;

// --- PUNTO CRITICO: Puntero al espacio de la MEMORIA VIRTUAL para los registros GPIO ---
// Basicamente, es la forma de acceder al hardware directamente
// La direccion fisica base de los puertos en una Raspberry Pi es 0xFE000000.
// El offset para el controlador GPIO es 0x200000
#define GPIO_BASE_PHYS  0xFE200000
#define GPIO_SIZE       0x84
static void __iomem *gpio_base_vaddr;

// --- Constantes de registros (para mayor claridad) ---
#define GPFSEL1_OFFSET 0x04
#define GPFSEL2_OFFSET 0x08
#define GPSET0_OFFSET  0x1C
#define GPCLR0_OFFSET  0x28
#define PULSE_MS       100

// Declaracion de las funciones de file_operations
static int      dev_open(struct inode *, struct file *);
static int      dev_release(struct inode *, struct file *);
static ssize_t  dev_write(struct file *, const char *, size_t, loff_t *);

// Estructura que asocia las funciones con las operaciones de archivo
static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .owner = THIS_MODULE,
};

// --- Funcion de inicializacion del modulo ---
static int __init robotic_hand_init(void) {
    printk(KERN_INFO "RoboticTEC Driver: Inicializando...\n");

    // 1. Obtener el major number de forma dinamica
    if (alloc_chrdev_region(&major_number, 0, 1, DEVICE_NAME) < 0) {
        printk(KERN_ALERT "RoboticTEC Driver: Fallo al asignar major number\n");
        return -1;
    }
    printk(KERN_INFO "RoboticTEC Driver: Major number %d asignado.\n", MAJOR(major_number));

    // 2. Crear una clase para dispositivo
    robotic_class = class_create(CLASS_NAME);
    if(IS_ERR(robotic_class)) {
        unregister_chrdev_region(major_number, 1);
        printk(KERN_ALERT "RoboticTEC Driver: Fallo al crear la clase de dispositivo\n");
        return PTR_ERR(robotic_class);
    }
    printk(KERN_INFO "RoboticTEC Driver: Clase de dispositivo creada.\n");

    // 3. Crear el dispositivo en /dev/robotic_hand
    robotic_device = device_create(robotic_class, NULL, major_number, NULL, DEVICE_NAME);
    if (IS_ERR(robotic_device)) {
        class_destroy(robotic_class);
        unregister_chrdev_region(major_number, 1);
        printk(KERN_ALERT "RoboticTEC Driver: Fallo al crear el dispositivo\n");
        return PTR_ERR(robotic_device);
    }
    printk(KERN_INFO "RoboticTEC Driver: Dispositivo /dev/robotic_hand creado.\n");

    // 4. Iniciar y agregar el dispositivo de caracteres (cdev)
    cdev_init(&my_cdev, &fops);
    if(cdev_add(&my_cdev, major_number, 1) < 0) {
        device_destroy(robotic_class, major_number);
        class_destroy(robotic_class);
        unregister_chrdev_region(major_number, 1);
        printk(KERN_ALERT "RoboticTEC Driver: Fallo al agregar cdev\n");
        return -1;
    }

    // 5. IMPORTANTE: Aqui se mapea la memoria FISICA a los registros GPIO a la memoria virtual del kernel
    gpio_base_vaddr = ioremap(GPIO_BASE_PHYS, GPIO_SIZE);
    if (!gpio_base_vaddr) {
        // Limpiar todo si falla el mapeo
        cdev_del(&my_cdev);
        device_destroy(robotic_class, major_number);
        class_destroy(robotic_class);
        unregister_chrdev_region(major_number, 1);
        printk(KERN_ALERT "RoboticTEC Driver: Fallo al mapear memoria GPIO (ioremap)\n");
        return -ENOMEM;
    }
    printk(KERN_INFO "RoboticTEC Driver: Memoria GPIO mapeada correctamente.\n");

    // Configurar los pines GPIO como salida
    
    printk(KERN_INFO "RoboticTEC Driver: Módulo cargado exitosamente.\n");
    return 0;
}

// --- Funcion de salida del modulo ---
static void __exit robotic_hand_exit(void) {
    printk(KERN_INFO "RoboticTEC Driver: Desinstalando módulo...\n");

    // Deshacer todo en orden inverso
    iounmap(gpio_base_vaddr);
    cdev_del(&my_cdev);
    device_destroy(robotic_class, major_number);
    class_destroy(robotic_class);
    unregister_chrdev_region(major_number, 1);

    printk(KERN_INFO "RoboticTEC Driver: Módulo desinstalado.\n");
}

// --- file ops ---
// cuando se hace open(/dev/robotic_hand)
static int dev_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "RoboticTEC Driver: Dispositivo abierto.\n");
    return 0;
}

// cuando se hace close()
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "RoboticTEC Driver: Dispositivo cerrado.\n");
    return 0;
}

// cuando se hace write()
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)  {
    char command;
    unsigned int reg_val;

    if (len == 0 || copy_from_user(&command, buffer, 1) != 0) {
        printk(KERN_ALERT "RoboticTEC: Error copiando datos desde el usuario\n");
        return -EFAULT; // Error al copiar desde el espacio de usuario
    }

    printk(KERN_INFO "RoboticTEC Driver: Recibido comando '%c'\n", command);

    // Aqui se manipulan directamente los registros GPIO
    // Segun la documentacion de BCM2711
    // * GPFSELn: Configura la función del pin (000 = entrada, 001 = salida)
    // * GPSETn: Pone un pin en ALTO (HIGH)
    // * GPCLRn: Pone un pin en BAJO (LOW)

    // Por ejemplo, para controlar GPIO 17
    // GPFSEL1 esta en offset 0x04, GPIO 17 se configura con los bits 23-21
    // Escribir '001' en esos bits lo configura como salida
    // iowrite32((ioread32(gpio_base_vaddr + 0x04) & ~(7 << 21)) | (1 << 21), gpio_base_vaddr + 0x04);
    
    // GPSET0 esta en offset 0x1C. Bit 17 para GPIO 17
    // iowrite32(1 << 17, gpio_base_vaddr + 0x1C);
    
    // GPCLR0 esta en offset 0x28. Bit 17 para GPIO 17
    // iowrite32(1 << 17, gpio_base_vaddr + 0x28);

    switch (command)
    {
    case 'A': // Adelante - GPIO 17
        reg_val = ioread32(gpio_base_vaddr + GPFSEL1_OFFSET);
        reg_val &= ~(7 << 21); // Limpiar los 3 bits (23, 22, 21)
        reg_val |= (1 << 21);  // Establecer el modo salida (001)
        iowrite32(reg_val, gpio_base_vaddr + GPFSEL1_OFFSET);
        
        // Generar pulso
        iowrite32(1 << 17, gpio_base_vaddr + GPSET0_OFFSET); // Poner en ALTO
        msleep(PULSE_MS);
        iowrite32(1 << 17, gpio_base_vaddr + GPCLR0_OFFSET); // Poner en BAJO
        break;
    
    case 'T': // aTras - GPIO 18
        // Configurar GPIO 18 como salida. Pertenece a GPFSEL1, bits 26-24.
        reg_val = ioread32(gpio_base_vaddr + GPFSEL1_OFFSET);
        reg_val &= ~(7 << 24); // Limpiar bits 26, 25, 24
        reg_val |= (1 << 24);  // Establecer modo salida (001)
        iowrite32(reg_val, gpio_base_vaddr + GPFSEL1_OFFSET);

        // Generar pulso
        iowrite32(1 << 18, gpio_base_vaddr + GPSET0_OFFSET); // ALTO
        msleep(PULSE_MS);
        iowrite32(1 << 18, gpio_base_vaddr + GPCLR0_OFFSET); // BAJO
        break;
    
    case 'D': // D Derecha - GPIO 27
        // Configurar GPIO 27 como salida. Pertenece a GPFSEL2, bits 23-21.
        reg_val = ioread32(gpio_base_vaddr + GPFSEL2_OFFSET);
        reg_val &= ~(7 << 21); // Limpiar bits 23, 22, 21
        reg_val |= (1 << 21);  // Establecer modo salida (001)
        iowrite32(reg_val, gpio_base_vaddr + GPFSEL2_OFFSET);

        // Generar pulso
        iowrite32(1 << 27, gpio_base_vaddr + GPSET0_OFFSET); // ALTO
        msleep(PULSE_MS);
        iowrite32(1 << 27, gpio_base_vaddr + GPCLR0_OFFSET); // BAJO
        break;

    case 'I': // Izquierda GPIO 22
        // Configurar GPIO 22 como salida. Pertenece a GPFSEL2, bits 8-6.
        reg_val = ioread32(gpio_base_vaddr + GPFSEL2_OFFSET);
        reg_val &= ~(7 << 6); // Limpiar bits 8, 7, 6
        reg_val |= (1 << 6);  // Establecer modo salida (001)
        iowrite32(reg_val, gpio_base_vaddr + GPFSEL2_OFFSET);

        // Generar pulso
        iowrite32(1 << 22, gpio_base_vaddr + GPSET0_OFFSET); // ALTO
        msleep(PULSE_MS);
        iowrite32(1 << 22, gpio_base_vaddr + GPCLR0_OFFSET); // BAJO
        break;
    
    case 'S':  // Subir GPIO 23
        // Configurar GPIO 23 como salida. Pertenece a GPFSEL2, bits 11-9.
        reg_val = ioread32(gpio_base_vaddr + GPFSEL2_OFFSET);
        reg_val &= ~(7 << 9); // Limpiar bits 11, 10, 9
        reg_val |= (1 << 9);  // Establecer modo salida (001)
        iowrite32(reg_val, gpio_base_vaddr + GPFSEL2_OFFSET);

        // Generar pulso
        iowrite32(1 << 23, gpio_base_vaddr + GPSET0_OFFSET); // ALTO
        msleep(PULSE_MS);
        iowrite32(1 << 23, gpio_base_vaddr + GPCLR0_OFFSET); // BAJO
        break;
    
    case 'B':   // Bajar - GPIO 24
        // Configurar GPIO 24 como salida. Pertenece a GPFSEL2, bits 14-12.
        reg_val = ioread32(gpio_base_vaddr + GPFSEL2_OFFSET);
        reg_val &= ~(7 << 12); // Limpiar bits 14, 13, 12
        reg_val |= (1 << 12);  // Establecer modo salida (001)
        iowrite32(reg_val, gpio_base_vaddr + GPFSEL2_OFFSET);
        
        // Generar pulso
        iowrite32(1 << 24, gpio_base_vaddr + GPSET0_OFFSET); // ALTO
        msleep(PULSE_MS);
        iowrite32(1 << 24, gpio_base_vaddr + GPCLR0_OFFSET); // BAJO
        break;
    
    default:
        printk(KERN_WARNING "RoboticTEC Driver: Comando no reconocido '%c'\n", command);
        break;
    }
    return 1; // Procesado 1 byte
}

module_init(robotic_hand_init);
module_exit(robotic_hand_exit);