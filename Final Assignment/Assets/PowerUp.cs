using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

public class PowerUp : NetworkBehaviour {

	// Use this for initialization
	void Start () {
		
	}
	
    [ClientRpc]
    public void RpcPickupPower (GameObject player)
    {
        //AttachFlagToGameObject(player);
    }

    /*public void AttachFlagToGameObject(GameObject obj)
    {
        obj.GetComponent
        glowObj.GetComponent<Renderer>().enabled = false;

    }*/
    void OnTriggerEnter(Collider other)
    {
        if (other.tag == "Player")
        {
            if (other.transform.GetComponent<PlayerController>().hasFlag)
            {
                Debug.Log("HAS FLAG");
            }
            else
                Debug.Log("NO FLAG");
        }


    }
}
