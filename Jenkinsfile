pipeline {
    agent {
        label "CPPDEV"
    }

    options {
        skipStagesAfterUnstable()
    }

    stages {
        stage("Build") {
            steps {
                sh '''#!/bin/bash

                '''
            }
        }
        stage("Send Report to Slack") {
            steps {
                slackSend(color: "good", message: "XRuntime Project Processed", channel: "#jenkins")
            }
        }
    }
}
