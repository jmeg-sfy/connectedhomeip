package com.google.chip.chiptool.clusterclient

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
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.launch


class WindowCoveringClientFragment : Fragment() {
  private val deviceController: ChipDeviceController
    get() = ChipClient.getDeviceController()

  private val scope = CoroutineScope(Dispatchers.Main + Job())

  private fun getPercent100thsText(percent : Int): String {
    val acc1 = percent / 100;
    val acc2 = percent  - acc1 * 100
    return "$acc1.$acc2"
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
      deviceController.setCompletionListener(ChipControllerCallback())

     updateAddressBtn.setOnClickListener { updateAddressClick() }
       downOrCloseBtn.setOnClickListener { scope.launch { sendDownOrCloseCommandClick() }}
          upOrOpenBtn.setOnClickListener { scope.launch { sendUpOrOpenCommandClick()    }}
        stopMotionBtn.setOnClickListener { scope.launch { sendStopMotionCommandClick()  }}
              readBtn.setOnClickListener { scope.launch { sendReadOnOffClick()          }}

      lvlPosTargetLift.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
          txtPosTargetLift.text = getPercent100thsText(lvlPosTargetLift.progress)
        }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) {
          scope.launch { goToLiftPercentage(lvlPosTargetLift.progress) }
        }
      })
      lvlPosTargetTilt.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) {
          txtPosTargetTilt.text = getPercent100thsText(lvlPosTargetTilt.progress)
        }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) {
          scope.launch { goToTiltPercentage(lvlPosTargetTilt.progress) }
        }
      })
      lvlPosCurrentLift.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
        override fun onProgressChanged(seekBar: SeekBar, i: Int, b: Boolean) { }
        override fun onStartTrackingTouch(seekBar: SeekBar?) { }
        override fun onStopTrackingTouch(seekBar: SeekBar?) { }
      })
      lvlPosCurrentTilt.isEnabled = false
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
          scope.launch { sendLevelCommandClick() }
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

    override fun onCommissioningComplete(nodeId: Long, errorCode: Int) {
      Log.d(TAG, "onCommissioningComplete for nodeId $nodeId: $errorCode")
    }

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

  override fun onStop() {
    super.onStop()
    scope.cancel()
  }

  private fun updateAddressClick() {
    try{
      deviceController.updateDevice(
          fabricIdEd.text.toString().toULong().toLong(),
          deviceIdEd.text.toString().toULong().toLong()
      )
      showMessage("Address update started")
    } catch (ex: Exception) {
      showMessage("Address update failed: $ex")
    }
  }

  private suspend fun sendLevelCommandClick() {
    val cluster = ChipClusters.LevelControlCluster(
      ChipClient.getConnectedDevicePointer(deviceIdEd.text.toString().toLong()), 1
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

  private suspend fun sendUpOrOpenCommandClick() {
    LAST_CMD = "UpOrOpen"
    try {
      getWindowCoveringClusterForDevice().upOrOpen(WindowCoveringClientFragment.ClusterCallback)
    } catch (ex: Exception) {
      showMessageCb("$LAST_CMD device com issue: $ex")
    }
  }

  private suspend fun sendDownOrCloseCommandClick() {
    LAST_CMD = "DownOrClose"
    try {
      getWindowCoveringClusterForDevice().downOrClose(WindowCoveringClientFragment.ClusterCallback)
    } catch (ex: Exception) {
      showMessageCb("$LAST_CMD device com issue: $ex")
    }
  }

  private suspend fun sendStopMotionCommandClick() {
    LAST_CMD = "StopMotion"
    try {
      getWindowCoveringClusterForDevice().stopMotion(WindowCoveringClientFragment.ClusterCallback)
    } catch (ex: Exception) {
      showMessageCb("$LAST_CMD device com issue: $ex")
    }
  }

  private suspend fun goToLiftPercentage(percent100ths : Int) {
    LAST_CMD = "GoToLift"
    try {
      getWindowCoveringClusterForDevice().goToLiftPercentage(WindowCoveringClientFragment.ClusterCallback, percent100ths / 100, percent100ths)
      Toast.makeText(requireContext(),"$LAST_CMD: " + getPercent100thsText(percent100ths) + " %", Toast.LENGTH_SHORT).show()
    } catch (ex: Exception) {
      showMessageCb("$LAST_CMD device com issue: $ex")
    }
  }

  private suspend fun goToTiltPercentage(percent100ths : Int) {
    LAST_CMD = "GoToTilt"
    try {
      getWindowCoveringClusterForDevice().goToTiltPercentage(WindowCoveringClientFragment.ClusterCallback, percent100ths / 100, percent100ths)
      Toast.makeText(requireContext(),"$LAST_CMD: " + getPercent100thsText(percent100ths) + " %", Toast.LENGTH_SHORT).show()
    } catch (ex: Exception) {
      showMessageCb("$LAST_CMD device com issue: $ex")
    }
  }

  private suspend fun sendReadOnOffClick() {
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

  private suspend fun getWindowCoveringClusterForDevice(): WindowCoveringCluster {
    return WindowCoveringCluster(
      ChipClient.getConnectedDevicePointer(deviceIdEd.text.toString().toLong()), 1
    )
  }

  private fun showMessage(msg: String) {
    requireActivity().runOnUiThread {
      commandStatusTv.text = msg
    }
  }

  companion object {
    var showMessageCb: (msg: String) -> Unit = { _ ->}
    private var LAST_CMD = "-"
    private const val TAG = "WindowCoveringClientFragment"
    fun newInstance(): WindowCoveringClientFragment = WindowCoveringClientFragment()
  }
}

