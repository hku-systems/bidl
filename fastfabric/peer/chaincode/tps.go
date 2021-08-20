package chaincode

import (
	"encoding/json"
	"fmt"
	"time"

	// "github.com/hyperledger/fabric/common/flogging"
	pb "github.com/hyperledger/fabric/protos/peer"

	log "github.com/sirupsen/logrus"


	"github.com/spf13/cobra"
)

// var logger = flogging.MustGetLogger("tps.go")


var chaincodeTpsCmd *cobra.Command


func tpsCmd(cf *ChaincodeCmdFactory) *cobra.Command {
	chaincodeTpsCmd = &cobra.Command{
		Use:       "tps",
		Short:     fmt.Sprintf("Test the tps of the specified %s.", chainFuncName),
		Long:      fmt.Sprintf("Test the tps of the specified %s. It will try to commit the endorsed transaction to the network.", chainFuncName),
		ValidArgs: []string{"1"},
		RunE: func(cmd *cobra.Command, args []string) error {
			return chaincodeTps(cmd, cf)
		},
	}
	flagList := []string{
		"tpsConfig",
	}
	attachFlags(chaincodeTpsCmd, flagList)

	return chaincodeTpsCmd
}

func chaincodeTps(cmd *cobra.Command, cf *ChaincodeCmdFactory) error {
	logger := log.New()
	logger.SetLevel(log.WarnLevel)

	cmd.SilenceUsage = true
	fmt.Printf("jyp: readFromConfigFile=%s\n", tpsConfigFile)

	config := LoadConfig(tpsConfigFile)
	// fmt.Println(config.Endorsers)

	N := config.TXs
	endorsementOnly := config.EndorsementOnly
	buffer := config.Buffer

	crypto := config.LoadCrypto()
	assember := &Assembler{Signer: crypto, idx: 0}
	raw := make(chan *Elements, buffer*10)
	signed := make([]chan *Elements, len(config.Endorsers))
	processed := make(chan *Elements, buffer*10)
	done := make(chan struct{})

	for i := 0; i < len(config.Endorsers); i++ {
		signed[i] = make(chan *Elements, buffer)
	}

	if endorsementOnly {
		for i := 0; i < config.NumOfAssembler; i++ {
			go assember.StartSigner(raw, signed, done, 0, 0)
		}
		proposor := CreateProposers(config.NumOfConn, config.ClientPerConn, config.Endorsers, logger)
		proposor.Start(signed, processed, done, config)
		chaincodeCtorJSONs := GenerateWorkload(N)
		start := time.Now()
		go func() {
			for i := 0; i < N; i++ {
				chaincodeCtorJSON := chaincodeCtorJSONs[i]
				input := &pb.ChaincodeInput{}
				if err := json.Unmarshal([]byte(chaincodeCtorJSON), &input); err != nil {
					return
				}
				prop := CreateProposal(
					crypto,
					config.Channel,
					config.Chaincode,
					config.Version,
					input,
				)
				raw <- &Elements{Proposal: prop}
			}

		}()

		cnt := 0
		for i := 0; i < N; i++ {
			select {
			case <-processed:
				cnt += 1
				if cnt % 1000 == 0 {
					fmt.Println("drop 1000 TXs")
				}
				if cnt >= N {
					break
				}
			}
		}

		duration := time.Since(start)
		fmt.Printf("tx: %d, duration: %+v, tps: %f\n", N, duration, float64(N)/duration.Seconds())
		close(done)

		return nil
	}

	envs := make(chan *Elements, buffer*10)


	for i := 0; i < config.NumOfAssembler; i++ {
		go assember.StartSigner(raw, signed, done, i, config.NumOfAssembler)
		go assember.StartIntegrator(processed, envs, done)
	}

	proposor := CreateProposers(config.NumOfConn, config.ClientPerConn, config.Endorsers, logger)
	proposor.Start(signed, processed, done, config)

	broadcaster := CreateBroadcasters(config.NumOfConnForOrderer, config.Orderer, logger)
	broadcaster.Start(envs, done)

	observer := CreateObserver(config.Channel, config.Committer, crypto, logger)

	// generate txns or readFromFile(TODO)
	chaincodeCtorJSONs := GenerateWorkload(N)

	start := time.Now()
	go observer.Start(N, start)

	perN := N / config.RawRoutine + 1
	for i := 0; i < config.RawRoutine; i++ {
		go func(idx int) {
			for j := 0; j < perN; j++ {
				id := idx * perN + j
				if id >= N {
					break
				}
				chaincodeCtorJSON := chaincodeCtorJSONs[id]
				input := &pb.ChaincodeInput{}
				if err := json.Unmarshal([]byte(chaincodeCtorJSON), &input); err != nil {
					return
				}
				prop := CreateProposal(
					crypto,
					config.Channel,
					config.Chaincode,
					config.Version,
					input,
				)
				raw <- &Elements{Proposal: prop}
			}
		}(i)
	}

	go func() {
		temp := len(config.Endorsers)
		for x := range time.Tick(time.Second) {
			fmt.Println(x)
			fmt.Printf("len(raw)=%d\nlen(processed)=%d\nlen(envs)=%d\nlen(signed)\n", len(raw), len(processed), len(envs))
			for i := 0; i < temp; i++ {
				fmt.Printf("signed[%d]=%d\n", i, len(signed[i]))
			}
		}
	}()

	observer.Wait()
	duration := time.Since(start)
	close(done)
	logger.Infof("Completed processing transactions.")
	fmt.Printf("tx: %d, duration: %+v, tps: %f\n", N, duration, float64(N)/duration.Seconds())
	return nil
}
