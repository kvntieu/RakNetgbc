using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class PowerUp : NetworkBehaviour {

	// Use this for initialization
	void Start () {
		
	}
	
    [ClientRpc]
    public void RpcPickupPower()
    {
        Destroy(gameObject);
    }
    [Command]
    public void CmdRpcPickupPower()
    {
        Destroy(gameObject);
        RpcPickupPower();
    }

    void OnTriggerEnter(Collider other) {

        if (other.tag == "Player")
        {
            CTFGameManager.powerUpCount--;
            CmdRpcPickupPower();
            other.transform.GetComponent<PlayerController>().isPoweredUp = true;
            if (other.transform.GetComponent<PlayerController>().hasFlag)
            {
                //spawn shield
                other.transform.GetComponent<PlayerController>().CmdEnableShield();
                other.transform.GetComponent<PlayerController>().powerType = "flag";
                Debug.Log("HAS FLAG");
                Debug.Log(other.transform.GetComponent<PlayerController>().powerType);
            }
            else
            {
                other.transform.GetComponent<PlayerController>().m_linearSpeed = 20.0f;
                other.transform.GetComponent<PlayerController>().powerType = "noFlag";
                Debug.Log("NO FLAG");
                Debug.Log(other.transform.GetComponent<PlayerController>().powerType);
            }
            
        }

    }
}
