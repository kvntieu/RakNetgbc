using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Networking;

public class CTFGameManager : NetworkBehaviour {

    public int m_numPlayers = 2;
    [SyncVar]
    float m_gameTime = 60.0f; // seconds
    [SyncVar]
    string inLead = "placeholder";

    public static int powerUpCount = 0;

    bool canSpawnPowerUp = true;

    float spawnTimer = 3.0f;

    public Text timeText;
    public Text inLeadText;
    public GameObject m_flag = null;
    public GameObject powerUpPrefab = null;
    bool gameStarted = false;
    public enum CTF_GameState
    {
        GS_WaitingForPlayers,
        GS_Ready,
        GS_InGame,
        GS_GameOver
    }

    [SyncVar]
    CTF_GameState m_gameState = CTF_GameState.GS_WaitingForPlayers;

    public bool SpawnFlag()
    {
    
        GameObject flag = Instantiate(m_flag, new Vector3(Random.Range(-40.0f, 40.0f), 2.0f, Random.Range(-40.0f, 40.0f)), new Quaternion());
        NetworkServer.Spawn(flag);
        return true;
    }

    bool IsNumPlayersReached()
    {
        return CTFNetworkManager.singleton.numPlayers == m_numPlayers;
    }

	// Use this for initialization
	void Start () {
       
    }
	
	// Update is called once per frame
	void Update ()
    {
	    if(isServer)
        {
            if (m_gameState == CTF_GameState.GS_WaitingForPlayers && IsNumPlayersReached())
            {
                m_gameState = CTF_GameState.GS_Ready;
            }
            if (m_gameState == CTF_GameState.GS_InGame)
            {
                m_gameTime -= Time.deltaTime;
                spawnTimer -= Time.deltaTime;
                
                if (m_gameTime <= 0.0f)
                {
                    m_gameState = CTF_GameState.GS_GameOver;
                }
                if (spawnTimer <= 0.0f)
                {
                    SpawnPowerUp();
                    spawnTimer = 3f;
                }

            }
            
        }
        timeText.text = m_gameTime.ToString("F0");
        UpdateGameState();
	}

    public void UpdateGameState()
    {
        if (m_gameState == CTF_GameState.GS_Ready)
        {
            //call whatever needs to be called
            if (isServer)
            {
                SpawnFlag();
                //change state to ingame
                m_gameState = CTF_GameState.GS_InGame;
            }
        }
        if (m_gameState == CTF_GameState.GS_GameOver)
        {
            timeText.text = "GAME OVER";
        }
        
    }

    public void GameOver() {

    }

    void SpawnPowerUp() {
        if (powerUpCount < 2)
        {
            Vector3 randomPoint = new Vector3(Random.Range(-40.0f, 40.0f), 0, Random.Range(-40.0f, 40.0f));
            GameObject powerUp = Instantiate(powerUpPrefab, randomPoint, new Quaternion());
            NetworkServer.Spawn(powerUp);
            powerUpCount++;
        }        
    }
    
}
