using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class Grab : NetworkBehaviour {

    private void OnTriggerStay(Collider other) {
        if (other.tag == "Player" && other.transform.GetComponent<PlayerController>().hasFlag) {
            Debug.Log(other.transform.GetComponent<PlayerController>().hasFlag);
           // if (other.transform.GetComponent<PlayerController>().hasFlag) {
                if (Input.GetKey(KeyCode.F))
                {
                    Debug.Log(GameObject.FindGameObjectWithTag("Flag"));
                    GameObject.FindGameObjectWithTag("Flag").GetComponent<Flag>().CmdDestroyFlag();
                    other.transform.GetComponent<PlayerController>().SetFlag(false);
                }
            //}
        }
    }
}
