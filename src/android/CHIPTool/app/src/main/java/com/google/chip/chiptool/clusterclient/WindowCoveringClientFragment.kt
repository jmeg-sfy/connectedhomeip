package com.google.chip.chiptool.clusterclient

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.SeekBar
import android.widget.Toast
import androidx.fragment.app.Fragment
import chip.devicecontroller.ChipCommandType
import chip.devicecontroller.ChipDeviceController
import chip.devicecontroller.ChipDeviceControllerException
import com.google.chip.chiptool.ChipClient
import com.google.chip.chiptool.GenericChipDeviceListener
import com.google.chip.chiptool.R
import com.google.chip.chiptool.util.DeviceIdUtil
import kotlinx.android.synthetic.main.window_covering_client_fragment.*
import kotlinx.android.synthetic.main.window_covering_client_fragment.view.*

//import kotlinx.android.synthetic.main.on_off_client_fragment.deviceIdEd
//import kotlinx.android.synthetic.main.on_off_client_fragment.view.*


class WindowCoveringClientFragment : Fragment() {
  private val TAG = "WC_Frag"
  private val deviceController: ChipDeviceController
    get() = ChipClient.getDeviceController()

  private var commandType: ChipCommandType? = null

  override fun onCreateView(
      inflater: LayoutInflater,
      container: ViewGroup?,
      savedInstanceState: Bundle?
  ): View {
    Log.i(TAG, "onCreate View")
    return inflater.inflate(R.layout.window_covering_client_fragment, container, false).apply {
      deviceController.setCompletionListener(ChipControllerCallback())

      downBtn.setOnClickListener { sendDownCommandClick() }
        upBtn.setOnClickListener { sendUpCommandClick() }
      stopBtn.setOnClickListener { sendStopCommandClick() }

    }
  }

  override fun onStart() {
    super.onStart()
    Log.i(TAG, "onStart beg")
    deviceIdEd.setText(DeviceIdUtil.getLastDeviceId(requireContext()).toString())
    Log.i(TAG, "onStart end")
    //ipAddressEd.setText(deviceController.ipAddress ?: requireContext().getString(R.string.enter_ip_address_hint_text))
  }

  inner class ChipControllerCallback : GenericChipDeviceListener() {
    override fun onConnectDeviceComplete() {
      sendCommand()
    }

    override fun onSendMessageComplete(message: String?) {
      windowCoveringCommandStatusTv.text = requireContext().getString(R.string.echo_status_response, message)
    }

    override fun onNotifyChipConnectionClosed() {
      Log.d(TAG, "onNotifyChipConnectionClosed")
    }

    override fun onCloseBleComplete() {
      Log.d(TAG, "onCloseBleComplete")
    }

    override fun onError(error: Throwable?) {
      Log.d(TAG, "onError: $error")
    }
  }

  private fun sendUpCommandClick() {
    commandType = ChipCommandType.UP
    sendCommand()
  }

  private fun sendStopCommandClick() {
    commandType = ChipCommandType.STOP
    sendCommand()
  }

  private fun sendDownCommandClick() {
    commandType = ChipCommandType.DOWN
    sendCommand()
  }

  private fun sendCommand() {
    val chipCommandType = commandType ?: run {
      Log.e(TAG, "No ChipCommandType specified.")
      return
    }

    windowCoveringCommandStatusTv.text =
        requireContext().getString(R.string.send_command_type_label_text_no_level, chipCommandType.name)

    try {
      // mask levelValue from integer to uint8_t and if null use 0
      deviceController.sendCommand(
        DeviceIdUtil.getLastDeviceId(requireContext()),
        commandType,
        0
      )
      //deviceController.beginSendCommand(commandType, 0)
    } catch (e: ChipDeviceControllerException) {
      windowCoveringCommandStatusTv.text = e.toString()
    }
  }

  companion object {
    private const val TAG = "WindowCoveringClientFragment"
    fun newInstance(): WindowCoveringClientFragment = WindowCoveringClientFragment()
  }
}
