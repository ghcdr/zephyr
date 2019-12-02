#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>


#define BT_ERROR(msg, code) \
            if(code != 0) \
                printk("\nBT_ERROR %d: %s\n", code, msg); \

#define BT_FATAL_ERROR(msg, code) \
            if(code != 0) \
            { \
                printk("\nBT_ERROR %d: %s\n", code, msg); \
                exit(1); \
            } \

void message(const char * msg)
{
    printk("\n%s\n", msg);
}

static void scan_callback(const bt_addr_le_t *addr, 
                    s8_t rssi, u8_t adv_type, 
                    struct net_buf_simple *buf)
{
}

static void auth_passkey_callback(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	printk("Pairing Complete\n");
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	printk("Pairing Failed (%d). Disconnecting.\n", reason);
	bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
}

static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);

	if (bt_conn_set_security(conn, BT_SECURITY_L4)) {
		printk("Failed to set security\n");
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);
}

static void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
			      const bt_addr_le_t *identity)
{
	char addr_identity[BT_ADDR_LE_STR_LEN];
	char addr_rpa[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
	bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));

	printk("Identity resolved %s -> %s\n", addr_rpa, addr_identity);
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level, err);
	}
}

static const struct bt_data adv_data[] = { BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),};

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.identity_resolved = identity_resolved,
	.security_changed = security_changed,
};

static struct bt_conn_auth_cb auth_callbacks = {
	.passkey_display = auth_passkey_callback,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

void main(void)
{
    {
        struct bt_le_scan_param scan_param = {};
        scan_param.type = BT_HCI_LE_SCAN_PASSIVE;
        scan_param.interval = 0x0010;
        scan_param.window = 0x0010;
        scan_param.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE;
        {
            message("Seting up.");
            BT_FATAL_ERROR("Bluetooth initialization", 
            bt_enable(NULL));
        }
        {
            message("scanning...");
            BT_FATAL_ERROR("Bluetooth initialization", 
            bt_le_scan_start(&scan_param, scan_callback));
        }
    }
    {
        bt_conn_auth_cb_register(&auth_callbacks);
	    bt_conn_cb_register(&conn_callbacks);
    }
    {
        BT_ERROR("Cannot advertise",
        bt_le_adv_start(BT_LE_ADV_CONN_NAME, adv_data, ARRAY_SIZE(adv_data), NULL, 0));
    }

}
