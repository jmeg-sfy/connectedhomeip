package com.google.chip.chiptool.clusterclient

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.SeekBar
import android.widget.Toast
import androidx.fragment.app.Fragment
import chip.devicecontroller.ChipClusters
import chip.devicecontroller.ChipClusters.WindowCoveringCluster
import chip.devicecontroller.ChipDeviceController
import chip.devicecontroller.ChipDeviceControllerException
import com.google.chip.chiptool.ChipClient
import com.google.chip.chiptool.GenericChipDeviceListener
import com.google.chip.chiptool.R
import com.google.chip.chiptool.util.DeviceIdUtil
import kotlinx.android.synthetic.main.on_off_client_fragment.*
import kotlinx.android.synthetic.main.window_covering_client_fragment.*
import kotlinx.android.synthetic.main.window_covering_client_fragment.commandStatusTv
import kotlinx.android.synthetic.main.window_covering_client_fragment.deviceIdEd
import kotlinx.android.synthetic.main.window_covering_client_fragment.fabricIdEd
import kotlinx.android.synthetic.main.window_covering_client_fragment.levelBar
import kotlinx.android.synthetic.main.window_covering_client_fragment.view.*


class WindowCoveringClientFragment : Fragment() {
  //private val TAG = "WC_Frag"
  //private val LAST_CMD = "-"
 // private val deviceController: ChipDeviceController get() = ChipClient.getDeviceController()

  private fun getPercent100thsText(percent : Int): String {
    val acc1 = percent / 100;
    val acc2 = percent  - acc1 * 100
    return "$acc1.$acc2"
  }

  private fun showMessage(msg: String) {
    requireActivity().runOnUiThread {
      commandStatusTv.text = msg
    }
  }

  object ClusterCallback : ChipClusters.DefaultClusterCallback {
    override fun onSuccess() {
      showMessageCb("$LAST_CMD command failure")
      Log.d(TAG, "$LAST_CMD command failure")
    }
    override fun onError(ex: Exception) {
      showMessageCb("$LAST_CMD command failure $ex")
      Log.e(TAG, "$LAST_CMD command failure", ex)
    }
  }

  override fun onCreateView(
    inflater: LayoutInflater,
    container: ViewGroup?,
    savedInstanceState: Bundle?
  ): View {
    Log.i(TAG, "onCreate View")
    showMessageCb = ::showMessage
    return inflater.inflate(R.layout.window_covering_client_fragment, container, false).apply {
      //deviceController.setCompletionListener(ChipControllerCallback())

     updateAddressBtn.setOnClickListener { updateAddressClick() }
       downOrCloseBtn.setOnClickListener { sendDownOrCloseCommandClick() }
          upOrOpenBtn.setOnClickListener { sendUpOrOpenCommandClick() }
        stopMotionBtn.setOnClickListener { sendStopMotionCommandClick() }
              readBtn.setOnClickListener { sendReadOnOffClick() }

      lvlPosTargetLift.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
          txtPosTargetLift.text = getPercent100thsText(lvlPosTargetLift.progress)
        }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) {
          goToLiftPercentage(lvlPosTargetLift.progress)
        }
      })
      lvlPosTargetTilt.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
          txtPosTargetTilt.text = getPercent100thsText(lvlPosTargetTilt.progress)
        }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) {
          goToTiltPercentage(lvlPosTargetTilt.progress)
        }
      })
      lvlPosCurrentLift.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) { }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) { }
      })
      lvlPosCurrentTilt.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) { }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) { }
      })
      levelBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {

        }

        override fun onStartTrackingTouch(seekBar: SeekBar?) {
        }

        override fun onStopTrackingTouch(seekBar: SeekBar?) {
          Toast.makeText(
            requireContext(),
            "Level is: " + levelBar.progress,
            Toast.LENGTH_SHORT
          ).show()
          sendLevelCommandClick()
        }
      })
    }
  }



  override fun onStart() {
    super.onStart()
    Log.i(TAG, "onStart beg")
    // TODO: use the fabric ID that was used to commission the device
    val testFabricId = "5544332211"
    fabricIdEd.setText(testFabricId)
    deviceIdEd.setText(DeviceIdUtil.getLastDeviceId(requireContext()).toString())
    Log.i(TAG, "onStart end")
  }

  inner class ChipControllerCallback : GenericChipDeviceListener() {
    override fun onConnectDeviceComplete() {}

    override fun onSendMessageComplete(message: String?) {
      commandStatusTv.text = requireContext().getString(R.string.echo_status_response, message)
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

  private fun getWindowCoveringClusterForDevice(): WindowCoveringCluster {
    return WindowCoveringCluster(ChipClient.getDeviceController().getDevicePointer(deviceIdEd.text.toString().toLong()), 1)
  }

  private fun updateAddressClick() {
    val serviceInfo = NsdServiceInfo().apply {
      serviceName = "%016X-%016X".format(
        fabricIdEd.text.toString().toLong(),
        deviceIdEd.text.toString().toLong()
      )
      serviceType = "_matter._tcp"
    }

    // TODO: implement the common CHIP mDNS interface for Android and make CHIP stack call the resolver
    val resolverListener = object : NsdManager.ResolveListener {
      override fun onResolveFailed(serviceInfo: NsdServiceInfo?, errorCode: Int) {
        showMessage("Address resolution failed: $errorCode")
      }

      override fun onServiceResolved(serviceInfo: NsdServiceInfo?) {
        val hostAddress = serviceInfo?.host?.hostAddress ?: ""
        val port = serviceInfo?.port ?: 0

        showMessage("Address: ${hostAddress}:${port}")

        if (hostAddress == "" || port == 0)
          return

        try {
         // deviceController.updateAddress(deviceIdEd.text.toString().toLong(), hostAddress, port)
        } catch (e: ChipDeviceControllerException) {
          showMessage(e.toString())
        }
      }
    }

    (requireContext().getSystemService(Context.NSD_SERVICE) as NsdManager).apply {
      resolveService(serviceInfo, resolverListener)
    }
  }

  private fun sendLevelCommandClick() {
    val cluster = ChipClusters.LevelControlCluster(
      ChipClient.getDeviceController()
        .getDevicePointer(deviceIdEd.text.toString().toLong()), 1
    )
    cluster.moveToLevel(object : ChipClusters.DefaultClusterCallback {
      override fun onSuccess() {
        showMessage("MoveToLevel command success")
      }

      override fun onError(ex: Exception) {
        showMessage("MoveToLevel command failure $ex")
        Log.e(TAG, "MoveToLevel command failure", ex)
      }

    }, levelBar.progress, 0, 0, 0)
  }

  private fun sendUpOrOpenCommandClick() {
    LAST_CMD = "UpOrOpen"
    getWindowCoveringClusterForDevice().upOrOpen(WindowCoveringClientFragment.ClusterCallback)
  }

  private fun sendDownOrCloseCommandClick() {
    LAST_CMD = "DownOrClose"
    getWindowCoveringClusterForDevice().downOrClose(WindowCoveringClientFragment.ClusterCallback)
  }

  private fun sendStopMotionCommandClick() {
    LAST_CMD = "GoTo Tilt"
    getWindowCoveringClusterForDevice().stopMotion(WindowCoveringClientFragment.ClusterCallback)
  }

  private fun goToLiftPercentage(percent100ths : Int) {
    LAST_CMD = "GoToLift"
    //getWindowCoveringClusterForDevice().goToLiftPercentage(WindowCoveringClientFragment.ClusterCallback, percent100ths / 100, percent100ths)
    Toast.makeText(requireContext(),"$LAST_CMD: " + getPercent100thsText(percent100ths) + " %", Toast.LENGTH_SHORT).show()
  }

  private fun goToTiltPercentage(percent100ths : Int) {
    LAST_CMD = "GoToTilt"
    //getWindowCoveringClusterForDevice().goToTiltPercentage(WindowCoveringClientFragment.ClusterCallback, percent100ths / 100, percent100ths)
    Toast.makeText(requireContext(),"$LAST_CMD: " + getPercent100thsText(percent100ths) + " %", Toast.LENGTH_SHORT).show()
  }

  private fun sendReadOnOffClick() {
    getWindowCoveringClusterForDevice().readTypeAttribute(object : ChipClusters.IntegerAttributeCallback {
      override fun onSuccess(type: Int) {
        Log.v(TAG, "Type attribute value: $type")
        showMessage("Type attribute value: $type")
      }

      override fun onError(ex: Exception) {
        Log.e(TAG, "Error reading Type attribute", ex)
      }
    })
  }

  companion object {
    var showMessageCb: (msg: String) -> Unit = { _ ->}
    private var LAST_CMD = "-"
    private const val TAG = "WindowCoveringClientFragment"
    fun newInstance(): WindowCoveringClientFragment = WindowCoveringClientFragment()
  }
}

