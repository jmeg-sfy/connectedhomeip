package com.google.chip.chiptool.clusterclient

import android.app.AlertDialog
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
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
    get() = ChipClient.getDeviceController(requireContext())

  private val scope = CoroutineScope(Dispatchers.Main + Job())

  private lateinit var addressUpdateFragment: AddressUpdateFragment

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

      addressUpdateFragment =
        childFragmentManager.findFragmentById(R.id.addressUpdateFragment) as AddressUpdateFragment


       downOrCloseBtn.setOnClickListener { scope.launch { sendDownOrCloseCommandClick() }}
          upOrOpenBtn.setOnClickListener { scope.launch { sendUpOrOpenCommandClick()    }}
        stopMotionBtn.setOnClickListener { scope.launch { sendStopMotionCommandClick()  }}
              readBtn.setOnClickListener { scope.launch { sendReadOnOffClick()          }}
showSubscribeDialogBtn.setOnClickListener { showSubscribeDialog() }

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


  private fun showSubscribeDialog() {
    val dialogView = requireActivity().layoutInflater.inflate(R.layout.subscribe_dialog, null)
    val dialog = AlertDialog.Builder(requireContext()).apply {
      setView(dialogView)
    }.create()

    val minIntervalEd = dialogView.findViewById<EditText>(R.id.minIntervalEd)
    val maxIntervalEd = dialogView.findViewById<EditText>(R.id.maxIntervalEd)
    dialogView.findViewById<Button>(R.id.subscribeBtn).setOnClickListener {
      scope.launch {
        sendSubscribeOnOffClick(
          minIntervalEd.text.toString().toInt(),
          maxIntervalEd.text.toString().toInt()
        )
        dialog.dismiss()
      }
    }
    dialog.show()
  }

  private suspend fun sendSubscribeOnOffClick(minInterval: Int, maxInterval: Int) {
    val onOffCluster = getOnOffClusterForDevice()

    val subscribeCallback = object : ChipClusters.DefaultClusterCallback {
      override fun onSuccess() {
        val message = "Subscribe on/off success"
        Log.v(TAG, message)
        showMessage(message)

        onOffCluster.reportOnOffAttribute(object : ChipClusters.BooleanAttributeCallback {
          override fun onSuccess(on: Boolean) {
            Log.v(TAG, "Report on/off attribute value: $on")

            val formatter = SimpleDateFormat("HH:mm:ss", Locale.getDefault())
            val time = formatter.format(Calendar.getInstance(Locale.getDefault()).time)
            showReportMessage("Report on/off at $time: ${if (on) "ON" else "OFF"}")
          }

          override fun onError(ex: Exception) {
            Log.e(TAG, "Error reporting on/off attribute", ex)
            showReportMessage("Error reporting on/off attribute: $ex")
          }
        })
      }

      override fun onError(ex: Exception) {
        Log.e(TAG, "Error configuring on/off attribute", ex)
      }
    }
    onOffCluster.subscribeOnOffAttribute(subscribeCallback, minInterval, maxInterval)
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
      showMessage("Address update complete for nodeId $nodeId with code $errorCode")
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

  private suspend fun sendLevelCommandClick() {
    val cluster = ChipClusters.LevelControlCluster(
      ChipClient.getConnectedDevicePointer(requireContext(), addressUpdateFragment.deviceId),
      WNCV_CONTROL_CLUSTER_ENDPOINT
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
      ChipClient.getConnectedDevicePointer(requireContext(), addressUpdateFragment.deviceId),
      WNCV_CLUSTER_ENDPOINT
    )
  }

  private fun showMessage(msg: String) {
    requireActivity().runOnUiThread {
      commandStatusTv.text = msg
    }
  }

  private fun showReportMessage(msg: String) {
    requireActivity().runOnUiThread {
      reportStatusTv.text = msg
    }
  }

  companion object {
    private const val TAG = "WindowCoveringClientFragment"

    private const val WNCV_CLUSTER_ENDPOINT = 1
    private const val LEVEL_CONTROL_CLUSTER_ENDPOINT = 1

    var showMessageCb: (msg: String) -> Unit = { _ ->}
    private var LAST_CMD = "-"

    fun newInstance(): WindowCoveringClientFragment = WindowCoveringClientFragment()
  }
}

