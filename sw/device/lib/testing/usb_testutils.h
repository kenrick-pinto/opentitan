// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#ifndef OPENTITAN_SW_DEVICE_LIB_TESTING_USB_TESTUTILS_H_
#define OPENTITAN_SW_DEVICE_LIB_TESTING_USB_TESTUTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sw/device/lib/dif/dif_usbdev.h"
#include "usb_testutils_diags.h"

typedef struct usb_testutils_ctx usb_testutils_ctx_t;

struct usb_testutils_ctx {
  dif_usbdev_t *dev;
  dif_usbdev_buffer_pool_t *buffer_pool;
  int flushed;
  /**
   * Have we received an indication of USB activity?
   */
  bool got_frame;
  /**
   * Most recent bus frame number received from host
   */
  uint16_t frame;

  /**
   * IN endpoints
   */
  struct {
    /**
     * Opaque context handle for callback functions
     */
    void *ep_ctx;
    /**
     * Callback for transmission of IN packet
     */
    void (*tx_done_callback)(void *);
    /**
     * Callback for periodically flushing IN data to host
     */
    void (*flush)(void *);
    /**
     * Callback for link reset
     */
    void (*reset)(void *);
  } in[USBDEV_NUM_ENDPOINTS];

  /**
   * OUT endpoints
   */
  struct {
    /**
     * Opaque context handle for callback functions
     */
    void *ep_ctx;
    /**
     * Callback for reception of IN packet
     */
    void (*rx_callback)(void *, dif_usbdev_rx_packet_info_t,
                        dif_usbdev_buffer_t);
    /**
     * Callback for link reset
     */
    void (*reset)(void *);
  } out[USBDEV_NUM_ENDPOINTS];
};

typedef enum usb_testutils_out_transfer_mode {
  /**
   * The endpoint does not support OUT transactions.
   */
  kUsbdevOutDisabled = 0,
  /**
   * Software does NOT need to call usb_testutils_clear_out_nak() after every
   * received transaction. If software takes no action, usbdev will allow an
   * endpoint's transactions to proceed as long as a buffer is available.
   */
  kUsbdevOutStream = 1,
  /**
   * Software must call usb_testutils_clear_out_nak() after every received
   * transaction to re-enable packet reception. This gives software time to
   * respond with the appropriate handshake when it's ready.
   */
  kUsbdevOutMessage = 2,
} usb_testutils_out_transfer_mode_t;

/**
 * Call to set up IN endpoint.
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 * @param ep_ctx context pointer for callee
 * @param tx_done(void *ep_ctx) callback once send has been Acked
 * @param flush(void *ep_ctx) called every 16ms based USB host timebase
 * @param reset(void *ep_ctx) called when an USB link reset is detected
 */
void usb_testutils_in_endpoint_setup(usb_testutils_ctx_t *ctx, uint8_t ep,
                                     void *ep_ctx, void (*tx_done)(void *),
                                     void (*flush)(void *),
                                     void (*reset)(void *));

/**
 * Call to set up OUT endpoint.
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 * @param out_mode the transfer mode for OUT transactions
 * @param ep_ctx context pointer for callee
 * @param rx(void *ep_ctx, usbbufid_t buf, int size, int setup)
          called when a packet is received
 * @param reset(void *ep_ctx) called when an USB link reset is detected
 */
void usb_testutils_out_endpoint_setup(
    usb_testutils_ctx_t *ctx, uint8_t ep,
    usb_testutils_out_transfer_mode_t out_mode, void *ep_ctx,
    void (*rx)(void *, dif_usbdev_rx_packet_info_t, dif_usbdev_buffer_t),
    void (*reset)(void *));

/**
 * Call to set up a pair of IN and OUT endpoints.
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 * @param out_mode the transfer mode for OUT transactions
 * @param ep_ctx context pointer for callee
 * @param tx_done(void *ep_ctx) callback once send has been Acked
 * @param rx(void *ep_ctx, usbbufid_t buf, int size, int setup)
          called when a packet is received
 * @param flush(void *ep_ctx) called every 16ms based USB host timebase
 * @param reset(void *ep_ctx) called when an USB link reset is detected
 */
void usb_testutils_endpoint_setup(usb_testutils_ctx_t *ctx, uint8_t ep,
                                  usb_testutils_out_transfer_mode_t out_mode,
                                  void *ep_ctx, void (*tx_done)(void *),
                                  void (*rx)(void *,
                                             dif_usbdev_rx_packet_info_t,
                                             dif_usbdev_buffer_t),
                                  void (*flush)(void *), void (*reset)(void *));

/**
 * Remove an IN endpoint.
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 */
void usb_testutils_in_endpoint_remove(usb_testutils_ctx_t *ctx, uint8_t ep);

/**
 * Remove an OUT endpoint.
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 */
void usb_testutils_out_endpoint_remove(usb_testutils_ctx_t *ctx, uint8_t ep);

/**
 * Remove a pair of IN and OUT endpoints
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 */
void usb_testutils_endpoint_remove(usb_testutils_ctx_t *ctx, uint8_t ep);

/**
 * Returns an indication of whether an endpoint is currently halted because
 * of the occurrence of an error.
 *
 * @param ctx usbdev context pointer
 * @param ep endpoint number
 * @return true iff the endpoint is halted as a result of an error condition
 */
inline bool usb_testutils_endpoint_halted(usb_testutils_ctx_t *ctx,
                                          dif_usbdev_endpoint_id_t endpoint);

/**
 * Initialize the usbdev interface
 *
 * Does not connect the device, since the default endpoint is not yet enabled.
 * See usb_testutils_connect().
 *
 * @param ctx uninitialized usbdev context pointer
 * @param pinflip boolean to indicate if PHY should be configured for D+/D- flip
 * @param en_diff_rcvr boolean to indicate if PHY should enable an external
 *                     differential receiver, activating the single-ended D
 *                     input
 * @param tx_use_d_se0 boolean to indicate if PHY uses D/SE0 for TX instead of
 *                     Dp/Dn
 */
void usb_testutils_init(usb_testutils_ctx_t *ctx, bool pinflip,
                        bool en_diff_rcvr, bool tx_use_d_se0);

/**
 * Call regularly to poll the usbdev interface
 *
 * @param ctx usbdev context pointer
 */
void usb_testutils_poll(usb_testutils_ctx_t *ctx);

/**
 * Finalize the usbdev interface
 *
 * @param ctx initialized usbdev context pointer
 */
void usb_testutils_fin(usb_testutils_ctx_t *ctx);

#endif  // OPENTITAN_SW_DEVICE_LIB_TESTING_USB_TESTUTILS_H_
